#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include <iostream>

#include "core/graph.hpp"
#include "parallel/sssp.hpp"

void compute(std::string path, VertexId vertices, VertexId root, int id)
{
    Graph<Weight> *graph;
    graph = new Graph<Weight>();
    graph->load_directed(argv[1], std::atoi(argv[2]));
    auto sssp = SSSP(id);
    sssp.compute(graph, root);
}

int main(int argc, char **argv)
{
    MPI_Instance mpi(&argc, &argv);

    if (argc < 3)
    {
        printf("sssp [file] [vertices]\n");
        exit(-1);
    }

    std::thread myThreads[8];
    for (int i = 0; i < 8; ++i)
    {
        myThreads[i] = std::thread(compute, argv[1], std::atoi(argv[2]), 211 * (i + 1), i);
    }

    for (int i = 0; i < 8; ++i)
    {
        myThreads[i].join();
    }

    return 0;
}