#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include <iostream>

#include "core/graph.hpp"
#include "parallel/sssp.hpp"
#include "parallel/bfs.hpp"
#include "parallel/pagerank.hpp"
#include "parallel/cc.hpp"

void computePR(std::string path, VertexId vertices, int id) // remember to change to Weight
{
    Graph<Weight> *graph;
    graph = new Graph<Weight>();
    graph->load_directed(path, vertices);
    auto pr = PageRank(id);
    pr.compute<Weight>(graph, 15);
}

void computeSSSP(std::string path, VertexId vertices, int id, VertexId root)
{
    Graph<Weight> *graph;
    graph = new Graph<Weight>();
    graph->load_directed(path, vertices);
    auto sssp = SSSP(id);
    sssp.compute(graph, root);
}

void computeBFS(std::string path, VertexId vertices, int id, VertexId root) // remember to change to Weight
{
    Graph<Weight> *graph;
    graph = new Graph<Weight>();
    graph->load_directed(path, vertices);
    auto bfs = BFS(id);
    bfs.compute<Weight>(graph, root);
}

void computeCC(std::string path, VertexId vertices, int id)
{
    Graph<Weight> *graph;
    graph = new Graph<Weight>();
    graph->load_directed(path, vertices);
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

    std::thread prThreads[2];
    std::thread bfsThreads[2];
    std::thread ssspThreads[2];
    std::thread ccThreads[2];
    for (int i = 0; i < 2; ++i)
    {
        bfsThreads[i] = std::thread(computeBFS, argv[1], std::atoi(argv[2]), 71 * (i + 1), 4 * i);
        ccThreads[i] = std::thread(computeCC, argv[1], std::atoi(argv[2]), 4 * i + 3);
        prThreads[i] = std::thread(computePR, argv[1], std::atoi(argv[2]), 4 * i + 2);
        ssspThreads[i] = std::thread(computeSSSP, argv[1], std::atoi(argv[2]), 101 * (i + 1) + 1, 4 * i + 1);
    }

    for (int i = 0; i < 2; ++i)
    {
        bfsThreads[i].join();
        ssspThreads[i].join();
        prThreads[i].join();
        ccThreads[i].join();
    }

    return 0;
}