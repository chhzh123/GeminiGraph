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

class BFS
{
public:
    BFS(int _label): label(_label){}

    template <typename M>
    void compute(Graph<M> *graph, VertexId root)
    {
        double exec_time = 0;
        exec_time -= get_time();

        VertexId *parent = graph->template alloc_vertex_array<VertexId>();
        VertexSubset *visited = graph->alloc_vertex_subset();
        VertexSubset *active_in = graph->alloc_vertex_subset();
        VertexSubset *active_out = graph->alloc_vertex_subset();

        visited->clear();
        visited->set_bit(root);
        active_in->clear();
        active_in->set_bit(root);
        graph->fill_vertex_array(parent, graph->vertices);
        parent[root] = root;

        VertexId active_vertices = 1;

        for (int i_i = 0; active_vertices > 0; i_i++)
        {
            if (graph->partition_id == 0)
            {
                printf("active(%d)>=%u\n", i_i, active_vertices);
            }
            active_out->clear();
            active_vertices = graph->template process_edges<VertexId, VertexId>(
                [&](VertexId src) {
                    graph->emit(src, src, label);
                },
                [&](VertexId src, VertexId msg, VertexAdjList<M> outgoing_adj) {
                    VertexId activated = 0;
                    for (AdjUnit<M> *ptr = outgoing_adj.begin; ptr != outgoing_adj.end; ptr++)
                    {
                        VertexId dst = ptr->neighbour;
                        if (parent[dst] == graph->vertices && cas(&parent[dst], graph->vertices, src))
                        {
                            active_out->set_bit(dst);
                            activated += 1;
                        }
                    }
                    return activated;
                },
                [&](VertexId dst, VertexAdjList<M> incoming_adj) {
                    if (visited->get_bit(dst))
                        return;
                    for (AdjUnit<M> *ptr = incoming_adj.begin; ptr != incoming_adj.end; ptr++)
                    {
                        VertexId src = ptr->neighbour;
                        if (active_in->get_bit(src))
                        {
                            graph->emit(dst, src, label);
                            break;
                        }
                    }
                },
                [&](VertexId dst, VertexId msg) {
                    if (cas(&parent[dst], graph->vertices, msg))
                    {
                        active_out->set_bit(dst);
                        return 1;
                    }
                    return 0;
                },
                active_in, nullptr, label);
            active_vertices = graph->template process_vertices<VertexId>(
                [&](VertexId vtx) {
                    visited->set_bit(vtx);
                    return 1;
                },
                active_out);
            std::swap(active_in, active_out);
        }

        exec_time += get_time();
        if (graph->partition_id == 0)
        {
            printf("exec_time=%lf(s)\n", exec_time);
        }

        graph->gather_vertex_array(parent, 0);
        if (graph->partition_id == 0)
        {
            VertexId found_vertices = 0;
            for (VertexId v_i = 0; v_i < graph->vertices; v_i++)
            {
                if (parent[v_i] < graph->vertices)
                {
                    found_vertices += 1;
                }
            }
            printf("found_vertices = %u\n", found_vertices);
        }

        graph->dealloc_vertex_array(parent);
        delete active_in;
        delete active_out;
        delete visited;
    }
    int label;
};
