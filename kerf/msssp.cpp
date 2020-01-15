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

#include "core/graph.hpp"

typedef float Weight;

class VecWeight
{
public:
    VecWeight() = default;
    VecWeight(const Weight val)
    {
        for (int i = 0; i < 8; ++i)
            weight[i] = val;
    }
    VecWeight(const VecWeight &other)
    {
        for (int i = 0; i < 8; ++i)
            weight[i] = other.weight[i];
    }
    Weight weight[8] = {0};
};

void compute(Graph<Weight> *graph)
{
    double exec_time = 0;
    exec_time -= get_time();

    Weight *distance[8];
    VertexSubset *active_in[8];
    VertexSubset *active_out[8];
    VertexId active_vertices = 8;
    VertexSubset *common_active_in = graph->alloc_vertex_subset();
    VertexSubset *common_active_out = graph->alloc_vertex_subset();
    VertexId root[8] = {211, 422, 633, 844, 1055, 1266, 1477, 1688};

    for (int i = 0; i < 8; ++i)
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
            for (int i = 0; i < 8; ++i)
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
        for (int i = 0; i < 8; ++i)
            active_out[i]->clear();
        common_active_out->clear();
        active_vertices = graph->process_edges<VertexId, VecWeight>(
            [&](VertexId src) {
                VecWeight vec((Weight)1e9);
                for (int i = 0; i < 8; ++i)
                    if (active_in[i]->get_bit(src))
                        vec.weight[i] = distance[i][src];
                graph->emit(src, vec);
            },
            [&](VertexId src, VecWeight msg, VertexAdjList<Weight> outgoing_adj) {
                VertexId activated = 0;
                for (AdjUnit<Weight> *ptr = outgoing_adj.begin; ptr != outgoing_adj.end; ptr++)
                {
                    VertexId dst = ptr->neighbour;
                    bool flag = false;
                    for (int i = 0; i < 8; ++i)
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
                    for (int i = 0; i < 8; ++i)
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
                    graph->emit(dst, msg);
            },
            [&](VertexId dst, VecWeight msg) {
                bool flag = false;
                for (int i = 0; i < 8; ++i)
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
            common_active_in);
        for (int i = 0; i < 8; ++i)
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

    for (int i = 0; i < 8; ++i)
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
        printf("sssp [file] [vertices]\n");
        exit(-1);
    }

    Graph<Weight> *graph;
    graph = new Graph<Weight>();
    graph->load_directed(argv[1], std::atoi(argv[2]));

    compute(graph);

    delete graph;
    return 0;
}
