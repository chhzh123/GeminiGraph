#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include <iostream>

#include "core/graph.hpp"
#include "concurrent/sssp.hpp"
#include "concurrent/pagerank.hpp"

void computePR(Graph<Weight> *graph, int id) // remember to change to Weight
{
    auto pr = PageRank(id);
    pr.compute<Weight>(graph, 10);
}

void computeSSSP(Graph<Weight> *graph, VertexId root, int id)
{
    auto sssp = SSSP(id);
    sssp.compute(graph, root);
}

int main(int argc, char **argv)
{
    MPI_Instance mpi(&argc, &argv);

    if (argc < 3)
    {
        printf("homo2 [file] [vertices]\n");
        exit(-1);
    }

    Graph<Weight> *graph;
    graph = new Graph<Weight>();
    graph->load_directed(argv[1], std::atoi(argv[2]));

    std::thread prThreads[4];
    std::thread ssspThreads[4];
    for (int i = 0; i < 4; ++i)
    {
        ssspThreads[i] = std::thread(computeSSSP, graph, 71 * (i+1) + 2, 2 * i + 1);
        prThreads[i] = std::thread(computePR, graph, 2 * i);
    }

    for (int i = 0; i < 4; ++i)
    {
        prThreads[i].join();
        ssspThreads[i].join();
    }

    delete graph;
    return 0;
}