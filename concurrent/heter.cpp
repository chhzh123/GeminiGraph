#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include <iostream>

#include "core/graph.hpp"
#include "concurrent/sssp.hpp"
#include "concurrent/bfs.hpp"
#include "concurrent/pagerank.hpp"
#include "concurrent/cc.hpp"

void computePR(Graph<Weight> *graph, int id) // remember to change to Weight
{
    auto pr = PageRank(id);
    pr.compute<Weight>(graph, 100);
}

void computeSSSP(Graph<Weight> *graph, VertexId root, int id)
{
    auto sssp = SSSP(id);
    sssp.compute(graph, root);
}

void computeBFS(Graph<Weight> *graph, VertexId root, int id) // remember to change to Weight
{
    auto bfs = BFS(id);
    bfs.compute<Weight>(graph, root);
}

void computeCC(Graph<Weight> *graph, int id)
{
    auto cc = CC(id);
    cc.compute(graph);
}

int main(int argc, char **argv)
{
    MPI_Instance mpi(&argc, &argv);

    if (argc < 3)
    {
        printf("heter [file] [vertices]\n");
        exit(-1);
    }

    Graph<Weight> *graph;
    graph = new Graph<Weight>();
    graph->load_directed(argv[1], std::atoi(argv[2]));

    std::thread prThreads[2];
    std::thread bfsThreads[2];
    std::thread ssspThreads[2];
    std::thread ccThreads[2];
    for (int i = 0; i < 2; ++i)
    {
        bfsThreads[i] = std::thread(computeBFS, graph, 71 * (i+1), 4 * i);
        ccThreads[i] = std::thread(computeCC, graph, 4 * i + 3);
        prThreads[i] = std::thread(computePR, graph, 4 * i + 2);
        ssspThreads[i] = std::thread(computeSSSP, graph, 101 * (i+1) + 1, 4 * i + 1);
    }

    for (int i = 0; i < 2; ++i)
    {
        bfsThreads[i].join();
        ssspThreads[i].join();
        prThreads[i].join();
        ccThreads[i].join();
    }

    delete graph;
    return 0;
}