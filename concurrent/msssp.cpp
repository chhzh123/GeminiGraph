#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include <iostream>

#include "core/graph.hpp"
#include "concurrent/sssp.hpp"

void compute(Graph<Weight> *graph, VertexId root, int id)
{
    auto sssp = SSSP(id);
    std::cout << root << std::endl;
    sssp.compute(graph, root);
}

int main(int argc, char **argv)
{
    MPI_Instance mpi(&argc, &argv);

    if (argc < 4)
    {
        printf("sssp [file] [vertices] [root]\n");
        exit(-1);
    }

    Graph<Weight> *graph;
    graph = new Graph<Weight>();
    VertexId root = std::atoi(argv[3]);
    graph->load_directed(argv[1], std::atoi(argv[2]));

    std::thread myThreads[8];
    for (int i = 0; i < 8; ++i)
    {
        myThreads[i] = std::thread(compute, graph, root, i);
    }

    for (int i = 0; i < 8; ++i)
    {
        myThreads[i].join();
    }

    delete graph;
    return 0;
}