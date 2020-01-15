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

class VecVertexId
{
public:
    VecVertexId() = default;
    VecVertexId(const VertexId val)
    {
        for (int i = 0; i < 8; ++i)
            vertex_id[i] = val;
    }
    VecVertexId(const VecVertexId& other)
    {
        for (int i = 0; i < 8; ++i)
            vertex_id[i] = other.vertex_id[i];
    }
    VertexId vertex_id[8] = {0};
};

void
compute(Graph<Empty> *graph)
{
    double exec_time = 0;
    exec_time -= get_time();

    VertexId *parent[8];
    VertexSubset *visited[8];
    VertexSubset *active_in[8];
    VertexSubset *active_out[8];
    VertexId active_vertices = 8;
    VertexSubset *common_active_in = graph->alloc_vertex_subset();
    VertexSubset *common_active_out = graph->alloc_vertex_subset();
    VertexId root[8] = {91,182,273,364,455,546,637,728};

    for (int i = 0; i < 8; ++i)
    {
        parent[i] = graph->alloc_vertex_array<VertexId>();
        visited[i] = graph->alloc_vertex_subset();
        active_in[i] = graph->alloc_vertex_subset();
        active_out[i] = graph->alloc_vertex_subset();

        visited[i]->clear();
        visited[i]->set_bit(root[i]);
        active_in[i]->clear();
        active_in[i]->set_bit(root[i]);
        graph->fill_vertex_array(parent[i], graph->vertices); // -1
        parent[i][root[i]] = root[i];

        common_active_in->set_bit(root[i]);
    }

    for (int i_i = 0; active_vertices > 0; i_i++)
    {
        if (graph->partition_id == 0)
        {
            for (int i = 0; i < 8; ++i)
            {
                int cnt = 0;
                for (size_t j = 0; j < active_in[i]->size; ++j)
                    if (active_in[i]->get_bit(j))
                        cnt++;
                printf("active task %d >= %u\n", i, cnt);
            }
            printf("active(%d)>=%u\n", i_i, active_vertices);
        }
        for (int i = 0; i < 8; ++i)
            active_out[i]->clear();
        common_active_out->clear();
        active_vertices = graph->process_edges<VertexId, VecVertexId>(
            [&](VertexId src) {
                VecVertexId vecId(graph->vertices);
                for (int i = 0; i < 8; ++i)
                    if (active_in[i]->get_bit(src))
                        vecId.vertex_id[i] = src;
                // VecVertexId vecId(src);
                graph->emit(src, vecId);
            },
            [&](VertexId src, VecVertexId msg, VertexAdjList<Empty> outgoing_adj) {
                VertexId activated = 0;
                for (AdjUnit<Empty> *ptr = outgoing_adj.begin; ptr != outgoing_adj.end; ptr++)
                {
                    VertexId dst = ptr->neighbour;
                    bool flag = false;
                    for (int i = 0; i < 8; ++i)
                    {
                        if (active_in[i]->get_bit(src) && parent[i][dst] == graph->vertices && cas(&parent[i][dst], graph->vertices, msg.vertex_id[i])) // be careful!
                        {
                            active_out[i]->set_bit(dst);
                            flag = true;
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
            [&](VertexId dst, VertexAdjList<Empty> incoming_adj) {
                // advanced task filter
                // int bit_mask = 0;
                bool bit_mask[8] = {0};
                int cnt = 0;
                for (int i = 0; i < 8; ++i)
                {
                    if (parent[i][dst] != graph->vertices) //(visited[i]->get_bit(dst))
                    {
                        // return;
                        // bit_mask &= 1 << i;
                        bit_mask[i] = 1;
                        cnt++;
                    }
                }
                if (cnt == 8) return; // all visited
                VecVertexId vecId(graph->vertices);
                bool flag = false;
                for (AdjUnit<Empty> *ptr = incoming_adj.begin; ptr != incoming_adj.end; ptr++)
                {
                    VertexId src = ptr->neighbour;
                    for (int i = 0; i < 8; ++i)
                    {
                        if (!bit_mask[i] && active_in[i]->get_bit(src)) // dst not visited & src active
                        {
                            vecId.vertex_id[i] = src;
                            bit_mask[i] = 1;
                            flag = true;
                        }
                    }
                }
                if (flag)
                    graph->emit(dst, vecId);
            },
            [&](VertexId dst, VecVertexId msg) {
                bool flag = false;
                for (int i = 0; i < 8; ++i)
                {
                    if (cas(&parent[i][dst], graph->vertices, msg.vertex_id[i]))
                    {
                        active_out[i]->set_bit(dst);
                        // return 1;
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
            common_active_in, nullptr);
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
        graph->gather_vertex_array(parent[i], 0);
        if (graph->partition_id == 0)
        {
            VertexId found_vertices = 0;
            for (VertexId v_i = 0; v_i < graph->vertices; v_i++)
            {
                if (parent[i][v_i] < graph->vertices)
                {
                    found_vertices += 1;
                }
            }
            printf("found_vertices = %u\n", found_vertices);
        }

        graph->dealloc_vertex_array(parent);
        delete active_in[i];
        delete active_out[i];
        delete visited[i];
    }
    delete common_active_in;
    delete common_active_out;
}

int main(int argc, char **argv)
{
    MPI_Instance mpi(&argc, &argv);

    if (argc < 3)
    {
        printf("mbfs [file] [vertices]\n");
        exit(-1);
    }

    Graph<Empty> *graph;
    graph = new Graph<Empty>();
    graph->load_directed(argv[1], std::atoi(argv[2]));

    compute(graph);

    delete graph;
    return 0;
}
