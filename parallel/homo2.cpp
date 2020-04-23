#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include <iostream>

#include "core/graph.hpp"
#include "parallel/sssp.hpp"
#include "parallel/pagerank.hpp"

void computePR(std::string path, VertexId vertices, int id) // remember to change to Weight
{
    Graph<Weight> *graph;
    graph = new Graph<Weight>();
    graph->load_directed(path, vertices);
    auto pr = PageRank(id);
    pr.compute<Weight>(graph, 15);
}

void computeSSSP(std::string path, VertexId vertices, VertexId root, int id)
{
    Graph<Weight> *graph;
    graph = new Graph<Weight>();
    graph->load_directed(path, vertices);
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

    std::thread prThreads[4];
    std::thread ssspThreads[4];
    for (int i = 0; i < 4; ++i)
    {
        ssspThreads[i] = std::thread(computeSSSP, argv[1], std::atoi(argv[2]), 71 * (i + 1) + 2, 2 * i + 1);
        prThreads[i] = std::thread(computePR, argv[1], std::atoi(argv[2]), 2 * i);
    }

    for (int i = 0; i < 4; ++i)
    {
        prThreads[i].join();
        ssspThreads[i].join();
    }

    return 0;
}