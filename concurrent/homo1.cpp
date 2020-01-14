#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include <iostream>

#include "core/graph.hpp"
#include "concurrent/cc.hpp"
#include "concurrent/bfs.hpp"

void computeBFS(Graph<Empty> *graph, VertexId root, int id)
{
    auto bfs = BFS(id);
    bfs.compute<Empty>(graph, root);
}

void computeCC(Graph<Empty> *graph, int id)
{
    auto cc = CC(id);
    cc.compute(graph);
}

int main(int argc, char **argv)
{
    MPI_Instance mpi(&argc, &argv);

    if (argc < 4)
    {
        printf("homo1 [file] [vertices] [root]\n");
        exit(-1);
    }

    Graph<Empty> *graph;
    graph = new Graph<Empty>();
    VertexId root = std::atoi(argv[3]);
    graph->load_directed(argv[1], std::atoi(argv[2]));

    std::thread bfsThreads[4];
    std::thread ccThreads[4];
    for (int i = 0; i < 4; ++i)
    {
        bfsThreads[i] = std::thread(computeBFS, graph, root, 2*i);
        ccThreads[i] = std::thread(computeCC, graph, 2*i+1);
    }

    for (int i = 0; i < 4; ++i)
    {
        bfsThreads[i].join();
        ccThreads[i].join();
    }

    delete graph;
    return 0;
}