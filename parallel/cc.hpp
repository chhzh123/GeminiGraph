/*
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

class CC
{
public:
    CC(int _id) : id(_id) {}

    template<typename M>
    void compute(Graph<M> *graph)
    {
        double exec_time = 0;
        exec_time -= get_time();

        VertexId *label = graph->template alloc_vertex_array<VertexId>();
        VertexSubset *active_in = graph->alloc_vertex_subset();
        active_in->fill();
        VertexSubset *active_out = graph->alloc_vertex_subset();

        VertexId active_vertices = graph->template process_vertices<VertexId>(
            [&](VertexId vtx) {
                label[vtx] = vtx;
                return 1;
            },
            active_in);

        for (int i_i = 0; active_vertices > 0; i_i++)
        {
            if (graph->partition_id == 0)
            {
                printf("active(%d)>=%u\n", i_i, active_vertices);
            }
            active_out->clear();
            active_vertices = graph->template process_edges<VertexId, VertexId>(
                [&](VertexId src) {
                    graph->emit(src, label[src], id);
                },
                [&](VertexId src, VertexId msg, VertexAdjList<M> outgoing_adj) {
                    VertexId activated = 0;
                    for (AdjUnit<M> *ptr = outgoing_adj.begin; ptr != outgoing_adj.end; ptr++)
                    {
                        VertexId dst = ptr->neighbour;
                        if (msg < label[dst])
                        {
                            write_min(&label[dst], msg);
                            active_out->set_bit(dst);
                            activated += 1;
                        }
                    }
                    return activated;
                },
                [&](VertexId dst, VertexAdjList<M> incoming_adj) {
                    VertexId msg = dst;
                    for (AdjUnit<M> *ptr = incoming_adj.begin; ptr != incoming_adj.end; ptr++)
                    {
                        VertexId src = ptr->neighbour;
                        if (label[src] < msg)
                        {
                            msg = label[src];
                        }
                    }
                    if (msg < dst)
                    {
                        graph->emit(dst, msg, id);
                    }
                },
                [&](VertexId dst, VertexId msg) {
                    if (msg < label[dst])
                    {
                        write_min(&label[dst], msg);
                        active_out->set_bit(dst);
                        return 1u;
                    }
                    return 0u;
                },
                active_in, nullptr, id);
            std::swap(active_in, active_out);
        }

        exec_time += get_time();
        if (graph->partition_id == 0)
        {
            printf("exec_time=%lf(s)\n", exec_time);
        }

        graph->gather_vertex_array(label, 0);
        if (graph->partition_id == 0)
        {
            VertexId *count = graph->template alloc_vertex_array<VertexId>();
            graph->fill_vertex_array(count, 0u);
            for (VertexId v_i = 0; v_i < graph->vertices; v_i++)
            {
                count[label[v_i]] += 1;
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

        graph->dealloc_vertex_array(label);
        delete active_in;
        delete active_out;
    }

    int id;
};