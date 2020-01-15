/*
Modified by Hongzheng Chen 2019-2020
    Added concurrent graph processing support

Copyright (c) 2014-2015 Xiaowei Zhu, Tsinghua University

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include <stdio.h>
#include <stdlib.h>
#include <thread>

#include "core/graph.hpp"

#include <math.h>

const double d = (double)0.85;

typedef float Weight;

class VecDouble
{
public:
    VecDouble() = default;
    VecDouble(const double val)
    {
        for (int i = 0; i < 4; ++i)
            data[i] = val;
    }
    VecDouble(const VecDouble &other)
    {
        for (int i = 0; i < 4; ++i)
            data[i] = other.data[i];
    }
    double data[4] = {0};
};

class VecWeight
{
public:
    VecWeight() = default;
    VecWeight(const Weight val)
    {
        for (int i = 0; i < 4; ++i)
            weight[i] = val;
    }
    VecWeight(const VecWeight &other)
    {
        for (int i = 0; i < 4; ++i)
            weight[i] = other.weight[i];
    }
    Weight weight[4] = {0};
};

void computePR(Graph<Weight> *graph, int id)
{
    int iterations = 10;
    double exec_time = 0;
    exec_time -= get_time();

    // the first 4 are pageranks
    double *curr[4];
    double *next[4];
    VertexSubset *active;

    // pr
    for (int i = 0; i < 4; ++i)
    {
        curr[i] = graph->alloc_vertex_array<double>();
        next[i] = graph->alloc_vertex_array<double>();
    }
    active = graph->alloc_vertex_subset();
    active->fill();

    double delta = graph->process_vertices<double>(
        [&](VertexId vtx) {
            for (int i = 0; i < 4; ++i){
                curr[i][vtx] = (double)1;
                if (graph->out_degree[vtx] > 0)
                {
                    curr[i][vtx] /= graph->out_degree[vtx];
                }
            }
            return (double)1;
        },
        active); // active has been filled
    delta /= graph->vertices;

    for (int i_i = 0; i_i < iterations; i_i++)
    {
        if (graph->partition_id == 0)
        {
            printf("delta(%d)=%lf\n", i_i, delta);
        }
        for (int i = 0; i < 4; ++i)
            graph->fill_vertex_array(next[i], (double)0);

        graph->process_edges<int, VecDouble>(
            [&](VertexId src) {
                VecDouble vec;
                for (int i = 0; i < 4; ++i)
                    vec.data[i] = curr[i][src];
                graph->emit(src, vec, id);
            },
            [&](VertexId src, VecDouble msg, VertexAdjList<Weight> outgoing_adj) {
                for (AdjUnit<Weight> *ptr = outgoing_adj.begin; ptr != outgoing_adj.end; ptr++)
                {
                    VertexId dst = ptr->neighbour;
                    for (int i = 0; i < 4; ++i)
                    {
                        write_add(&next[i][dst], msg.data[i]);
                    }
                }
                return 0;
            },
            [&](VertexId dst, VertexAdjList<Weight> incoming_adj) {
                VecDouble sum(0);
                for (AdjUnit<Weight> *ptr = incoming_adj.begin; ptr != incoming_adj.end; ptr++)
                {
                    VertexId src = ptr->neighbour;
                    for (int i = 0; i < 4; ++i)
                    {
                        sum.data[i] += curr[i][src];
                    }
                }
                graph->emit(dst, sum, id);
            },
            [&](VertexId dst, VecDouble msg) {
                for (int i = 0; i < 4; ++i)
                    write_add(&next[i][dst], msg.data[i]);
                return 0;
            },
            active, nullptr, id);
        if (i_i == iterations - 1)
        {
            delta = graph->process_vertices<double>(
                [&](VertexId vtx) {
                    for (int i = 0; i < 4; ++i)
                    {
                        next[i][vtx] = 1 - d + d * next[i][vtx];
                    }
                    return 0;
                },
                active);
        }
        else
        {
            delta = graph->process_vertices<double>(
                [&](VertexId vtx) {
                    bool flag = false;
                    for (int i = 0; i < 4; ++i)
                    {
                        next[i][vtx] = 1 - d + d * next[i][vtx];
                        if (graph->out_degree[vtx] > 0)
                        {
                            next[i][vtx] /= graph->out_degree[vtx];
                            flag = true;
                        }
                    }
                    if (flag)
                        return fabs(next[0][vtx] - curr[0][vtx]) * graph->out_degree[vtx];
                    else
                        return fabs(next[0][vtx] - curr[0][vtx]);
                },
                active);
        }
        delta /= graph->vertices;
        for (int i = 0; i < 4; ++i)
            std::swap(curr[i], next[i]);
    }

    exec_time += get_time();
    if (graph->partition_id == 0)
    {
        printf("exec_time=%lf(s)\n", exec_time);
    }

    for (int i = 0; i < 4; ++i)
    {
        double pr_sum = graph->process_vertices<double>(
            [&](VertexId vtx) {
                return curr[i][vtx];
            },
            active);
        if (graph->partition_id == 0)
        {
            printf("pr_sum=%lf\n", pr_sum);
        }

        graph->gather_vertex_array(curr, 0);
        if (graph->partition_id == 0)
        {
            VertexId max_v_i = 0;
            for (VertexId v_i = 0; v_i < graph->vertices; v_i++)
            {
                if (curr[v_i] > curr[max_v_i])
                    max_v_i = v_i;
            }
            printf("pr[%u]=%lf\n", max_v_i, curr[max_v_i]);
        }

        graph->dealloc_vertex_array(curr[i]);
        graph->dealloc_vertex_array(next[i]);
    }
    delete active;
}

void computeSSSP(Graph<Weight> *graph, int id)
{
    double exec_time = 0;
    exec_time -= get_time();

    // the latter 4 are sssps
    Weight *distance[4];
    VertexSubset *active_in[4];
    VertexSubset *active_out[4];
    VertexId active_vertices = 4;
    VertexSubset *common_active_in = graph->alloc_vertex_subset();
    VertexSubset *common_active_out = graph->alloc_vertex_subset();
    VertexId root[4] = {73, 144, 215, 286};

    for (int i = 0; i < 4; ++i)
    {
        distance[i] = graph->alloc_vertex_array<Weight>();
        active_in[i] = graph->alloc_vertex_subset();
        active_out[i] = graph->alloc_vertex_subset();
        active_in[i]->clear();
        active_in[i]->set_bit(root[i]);
        graph->fill_vertex_array(distance[i], (Weight)1e9); // initialization
        distance[i][root[i]] = (Weight)0;

        common_active_in->set_bit(root[i]);
    }

    for (int i_i = 0; active_vertices > 0; i_i++)
    {
        if (graph->partition_id == 0)
        {
#ifdef PRINT_DEBUG_MESSAGES
            for (int i = 0; i < 4; ++i)
            {
                int cnt = 0;
                for (size_t j = 0; j < active_in[i]->size; ++j)
                    if (active_in[i]->get_bit(j))
                        cnt++;
                printf("active task %d >= %u\n", i, cnt);
            }
#endif
            printf("active(%d)>=%u\n", i_i, active_vertices);
        }
        for (int i = 0; i < 4; ++i)
            active_out[i]->clear();
        common_active_out->clear();
        active_vertices = graph->process_edges<VertexId, VecWeight>(
            [&](VertexId src) {
                VecWeight vec((Weight)1e9);
                for (int i = 0; i < 8; ++i)
                    if (active_in[i]->get_bit(src))
                        vec.weight[i] = distance[i][src];
                graph->emit(src, vec, id);
            },
            [&](VertexId src, VecWeight msg, VertexAdjList<Weight> outgoing_adj) {
                VertexId activated = 0;
                for (AdjUnit<Weight> *ptr = outgoing_adj.begin; ptr != outgoing_adj.end; ptr++)
                {
                    VertexId dst = ptr->neighbour;
                    bool flag = false;
                    for (int i = 0; i < 4; ++i)
                    {
                        Weight relax_dist = msg.weight[i] + ptr->edge_data;
                        if (active_in[i]->get_bit(src))
                        {
                            if (relax_dist < distance[i][dst])
                            {
                                if (write_min(&distance[i][dst], relax_dist))
                                {
                                    active_out[i]->set_bit(dst);
                                    flag = true;
                                }
                            }
                        }
                    }
                    if (flag)
                    {
                        activated += 1;
                        common_active_out->set_bit(dst);
                    }
                }
                return activated;
            },
            [&](VertexId dst, VertexAdjList<Weight> incoming_adj) {
                bool flag = false;
                VecWeight msg(1e9);
                for (AdjUnit<Weight> *ptr = incoming_adj.begin; ptr != incoming_adj.end; ptr++)
                {
                    VertexId src = ptr->neighbour;
                    for (int i = 0; i < 4; ++i)
                    {
                        // if (active_in->get_bit(src)) {
                        Weight relax_dist = distance[i][src] + ptr->edge_data;
                        if (relax_dist < msg.weight[i])
                        {
                            msg.weight[i] = relax_dist;
                            flag = true;
                        }
                        // }
                    }
                }
                if (flag)
                    graph->emit(dst, msg, id);
            },
            [&](VertexId dst, VecWeight msg) {
                bool flag = false;
                for (int i = 0; i < 4; ++i)
                {
                    if (msg.weight[i] < distance[i][dst])
                    {
                        write_min(&distance[i][dst], msg.weight[i]);
                        active_out[i]->set_bit(dst);
                        flag = true;
                    }
                }
                if (flag)
                {
                    common_active_out->set_bit(dst);
                    return 1;
                }
                else
                    return 0;
            },
            common_active_in, nullptr, id);
        for (int i = 0; i < 4; ++i)
        {
            std::swap(active_in[i], active_out[i]);
        }
        std::swap(common_active_in, common_active_out);
    }

    exec_time += get_time();
    if (graph->partition_id == 0)
    {
        printf("exec_time=%lf(s)\n", exec_time);
    }

    for (int i = 0; i < 4; ++i)
    {
        graph->gather_vertex_array(distance[i], 0);
        if (graph->partition_id == 0)
        {
            VertexId max_v_i = root[i];
            for (VertexId v_i = 0; v_i < graph->vertices; v_i++)
            {
                if (distance[i][v_i] < 1e9 && distance[i][v_i] > distance[i][max_v_i])
                {
                    max_v_i = v_i;
                }
            }
            printf("distance[%u]=%f\n", max_v_i, distance[i][max_v_i]);
        }
        graph->dealloc_vertex_array(distance[i]);
        delete active_in[i];
        delete active_out[i];
    }
    delete common_active_in;
    delete common_active_out;
}

int main(int argc, char **argv)
{
    MPI_Instance mpi(&argc, &argv);

    if (argc < 3)
    {
        printf("homo2 [file] [vertices]\n");
        exit(-1);
    }

    Graph<Weight> *graph;
    graph = new Graph<Weight>();
    graph->load_directed(argv[1], std::atoi(argv[2]));

    std::thread t1(computePR,graph,0);
    std::thread t2(computeSSSP,graph,1);

    t1.join();
    t2.join();

    delete graph;
    return 0;
}
