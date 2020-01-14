#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include <iostream>

#include "core/graph.hpp"
#include "concurrent/sssp.hpp"
#include "concurrent/bfs.hpp"

void computeBFS(Graph<Weight> *graph, VertexId root, int id) // remember to change to Weight
{
    auto bfs = BFS(id);
    bfs.compute<Weight>(graph, root);
}

void computeSSSP(Graph<Weight> *graph, VertexId root, int id)
{
    auto sssp = SSSP(id);
    sssp.compute(graph, root);
}

int main(int argc, char **argv)
{
    MPI_Instance mpi(&argc, &argv);

    if (argc < 4)
    {
        printf("homo1 [file] [vertices] [root]\n");
        exit(-1);
    }

    Graph<Weight> *graph;
    graph = new Graph<Weight>();
    VertexId root = std::atoi(argv[3]);
    graph->load_directed(argv[1], std::atoi(argv[2]));

    std::thread bfsThreads[4];
    std::thread ssspThreads[4];
    for (int i = 0; i < 4; ++i)
    {
        bfsThreads[i] = std::thread(computeBFS, graph, root, 2*i);
        ssspThreads[i] = std::thread(computeSSSP, graph, root, 2*i+1);
    }

    for (int i = 0; i < 4; ++i)
    {
        bfsThreads[i].join();
        ssspThreads[i].join();
    }

    delete graph;
    return 0;
}