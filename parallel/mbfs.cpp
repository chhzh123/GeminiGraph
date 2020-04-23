#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include <iostream>

#include "core/graph.hpp"
#include "parallel/bfs.hpp"

void compute(std::string path, VertexId vertices, VertexId root, int id)
{
    Graph<Empty> *graph;
    graph = new Graph<Empty>();
    graph->load_directed(path, vertices);
    auto bfs = BFS(id);
    bfs.compute<Empty>(graph,root);
}

int main(int argc, char **argv)
{
    MPI_Instance mpi(&argc, &argv);

    if (argc < 3)
    {
        printf("bfs [file] [vertices]\n");
        exit(-1);
    }

    std::thread myThreads[8];
    for (int i = 0; i < 8; ++i) {
        myThreads[i] = std::thread(compute, argv[1], std::atoi(argv[2]), 91 * (i + 1), i);
    }

    for (int i = 0; i < 8; ++i) {
        myThreads[i].join();
    }

    return 0;
}