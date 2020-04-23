#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include <iostream>

#include "core/graph.hpp"
#include "parallel/cc.hpp"
#include "parallel/bfs.hpp"

void computeBFS(std::string path, VertexId vertices, VertexId root, int id)
{
    Graph<Empty> *graph;
    graph = new Graph<Empty>();
    graph->load_directed(path, vertices);
    auto bfs = BFS(id);
    bfs.compute<Empty>(graph, root);
}

void computeCC(std::string path, VertexId vertices, int id)
{
    Graph<Empty> *graph;
    graph = new Graph<Empty>();
    graph->load_directed(path, vertices);
    auto cc = CC(id);
    cc.compute(graph);
}

int main(int argc, char **argv)
{
    MPI_Instance mpi(&argc, &argv);

    if (argc < 3)
    {
        printf("homo1 [file] [vertices]\n");
        exit(-1);
    }

    std::thread bfsThreads[4];
    std::thread ccThreads[4];
    for (int i = 0; i < 4; ++i)
    {
        bfsThreads[i] = std::thread(computeBFS, argv[1], std::atoi(argv[2]), 10 * (i + 1), 2 * i);
        ccThreads[i] = std::thread(computeCC, argv[1], std::atoi(argv[2]), 2 * i + 1);
    }

    for (int i = 0; i < 4; ++i)
    {
        bfsThreads[i].join();
        ccThreads[i].join();
    }

    return 0;
}