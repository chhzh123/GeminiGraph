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

class VecMix
{
public:
    VecMix() = default;
    VecMix(const VertexId val)
    {
        for (int i = 0; i < 8; ++i)
            data[i] = val;
    }
    VecMix(const VecMix &other)
    {
        for (int i = 0; i < 8; ++i)
            data[i] = other.data[i];
    }
    VertexId data[8] = {0};
};

void compute(Graph<Empty> *graph)
{
    double exec_time = 0;
    exec_time -= get_time();

    // the first 4 are BFSs
    VertexId *parent[8];
    VertexId root[4] = {10, 20, 30, 40};
    VertexId active_vertices = 4;
    // the latter 4 are CCs
    VertexId *label[4];
    VertexSubset *active_in[8];
    VertexSubset *active_out[8];
    VertexSubset *common_active_in = graph->alloc_vertex_subset();
    VertexSubset *common_active_out = graph->alloc_vertex_subset();

    for (int i = 0; i < 8; ++i)
    {
        if (i < 4)
            parent[i] = graph->alloc_vertex_array<VertexId>();
        else
            label[i - 4] = graph->alloc_vertex_array<VertexId>();
        active_in[i] = graph->alloc_vertex_subset();
        active_out[i] = graph->alloc_vertex_subset();
        if (i < 4)
        {
            active_in[i]->clear();
            active_in[i]->set_bit(root[i]);
            graph->fill_vertex_array(parent[i], graph->vertices);
            parent[i][root[i]] = root[i];
            common_active_in->set_bit(root[i]);
        }
        else
        {
            active_in[i]->fill();
            common_active_in->fill();
        }
    }

    // cc
    active_vertices = graph->process_vertices<VertexId>(
        [&](VertexId vtx) {
            for (int i = 0; i < 4; ++i)
                label[i][vtx] = vtx;
            return 1;
        },
        common_active_in);

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

        active_vertices = graph->process_edges<VertexId, VecMix>(
            [&](VertexId src) {
                VecMix vec;
                for (int i = 0; i < 8; ++i)
                    if (active_in[i]->get_bit(src))
                        vec.data[i] = (i < 4 ? src : label[i - 4][src]);
                graph->emit(src, vec);
            },
            [&](VertexId src, VecMix msg, VertexAdjList<Empty> outgoing_adj) {
                VertexId activated = 0;
                for (AdjUnit<Empty> *ptr = outgoing_adj.begin; ptr != outgoing_adj.end; ptr++)
                {
                    VertexId dst = ptr->neighbour;
                    bool flag = false;
                    for (int i = 0; i < 4; ++i) // bfs
                    {
                        if (active_in[i]->get_bit(src) && parent[i][dst] == graph->vertices && cas(&parent[i][dst], graph->vertices, msg.data[i])) // be careful!
                        {
                            active_out[i]->set_bit(dst);
                            flag = true;
                        }
                    }
                    for (int i = 0; i < 4; ++i) // cc
                    {
                        if (active_in[i + 4]->get_bit(src) && msg.data[i + 4] < label[i][dst])
                        {
                            write_min(&label[i][dst], msg.data[i + 4]);
                            active_out[i + 4]->set_bit(dst);
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
                // bfs
                bool bit_mask[4] = {0};
                int cnt = 0;
                for (int i = 0; i < 4; ++i)
                {
                    if (parent[i][dst] != graph->vertices) //(visited[i]->get_bit(dst))
                    {
                        // return;
                        // bit_mask &= 1 << i;
                        bit_mask[i] = 1;
                        cnt++;
                    }
                }

                VecMix msg(graph->vertices);
                for (int i = 4; i < 8; ++i)
                    msg.data[i] = dst;
                bool flag = false;
                // traversal
                for (AdjUnit<Empty> *ptr = incoming_adj.begin; ptr != incoming_adj.end; ptr++)
                {
                    VertexId src = ptr->neighbour;
                    for (int i = 0; i < 4; ++i)
                    {
                        if (!bit_mask[i] && active_in[i]->get_bit(src)) // dst not visited & src active
                        {
                            msg.data[i] = src;
                            bit_mask[i] = 1;
                            flag = true;
                        }
                    }
                    for (int i = 4; i < 8; ++i) // cc
                    {
                        if (label[i - 4][src] < msg.data[i] && active_in[i]->get_bit(src))
                        {
                            msg.data[i] = label[i - 4][src];
                            flag = true;
                        }
                    }
                }
                if (flag)
                    graph->emit(dst, msg);
            },
            [&](VertexId dst, VecMix msg) {
                bool flag = false;
                for (int i = 0; i < 4; ++i)
                {
                    if (cas(&parent[i][dst], graph->vertices, msg.data[i]))
                    {
                        active_out[i]->set_bit(dst);
                        // return 1;
                        flag = true;
                    }
                }
                for (int i = 4; i < 8; ++i)
                {
                    if (msg.data[i] < label[i - 4][dst])
                    {
                        write_min(&label[i - 4][dst], msg.data[i]);
                        active_out[i]->set_bit(dst);
                        flag = true;
                    }
                }
                if (flag)
                {
                    common_active_out->set_bit(dst);
                    return 1u;
                }
                else
                    return 0u;
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

    // output
    for (int i = 0; i < 4; ++i)
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
    }
    for (int i = 4; i < 8; ++i)
    {
        graph->gather_vertex_array(label[i - 4], 0);
        if (graph->partition_id == 0)
        {
            VertexId *count = graph->alloc_vertex_array<VertexId>();
            graph->fill_vertex_array(count, 0u);
            for (VertexId v_i = 0; v_i < graph->vertices; v_i++)
            {
                count[label[i - 4][v_i]] += 1;
            }
            VertexId components = 0;
            for (VertexId v_i = 0; v_i < graph->vertices; v_i++)
            {
                if (count[v_i] > 0)
                {
                    components += 1;
                }
            }
            printf("components = %u\n", components);
        }
        graph->dealloc_vertex_array(label[i - 4]);
    }

    for (int i = 0; i < 8; ++i)
    {
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
        printf("homo1 [file] [vertices]\n");
        exit(-1);
    }

    Graph<Empty> *graph;
    graph = new Graph<Empty>();
    graph->load_directed(argv[1], std::atoi(argv[2]));

    compute(graph);

    delete graph;
    return 0;
}