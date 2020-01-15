ROOT_DIR= $(shell pwd)
TARGETS= toolkits/bc toolkits/bfs toolkits/cc toolkits/pagerank toolkits/sssp
CON_TARGETS= concurrent/homo1 concurrent/homo2 concurrent/heter concurrent/mbfs concurrent/msssp
KERF_TARGETS= kerf/mbfs
MACROS=
# MACROS= -D PRINT_DEBUG_MESSAGES

MPICXX= mpicxx
CXXFLAGS= -O3 -Wall -std=c++11 -g -fopenmp -march=native -I$(ROOT_DIR) $(MACROS)
SYSLIBS= -lnuma
HEADERS= $(shell find . -name '*.hpp')

DATASET_PATH = dataset
PROFILE_PATH = profile

ifdef CP
DATA = cit-Patents
DATASET = $(DATASET_PATH)/$(DATA).in
DATASETW = $(DATASET_PATH)/$(DATA)-w.in
SIZE = 6009555

else ifdef LJ
DATA = soc-LiveJournal1
DATASET = $(DATASET_PATH)/$(DATA).in
DATASETW = $(DATASET_PATH)/$(DATA)-w.in
SIZE = 4847571

else ifdef RMAT
DATA = rMatGraph24
DATASET = $(DATASET_PATH)/$(DATA).in
DATASETW = $(DATASET_PATH)/$(DATA)-w.in
SIZE = 33554432

else ifdef TW
DATA = twitter7
DATASET = $(DATASET_PATH)/$(DATA).in
DATASETW = $(DATASET_PATH)/$(DATA)-w.in
SIZE = 41652231

else ifdef FT
DATA = com-friendster
DATASET = $(DATASET_PATH)/$(DATA).in
DATASETW = $(DATASET_PATH)/$(DATA)-w.in

else
DATA = rMatGraph
DATASET = $(DATASET_PATH)/$(DATA)_J_5_100.in
DATASETW = $(DATASET_PATH)/$(DATA)_WJ_5_100.in
endif

all: $(TARGETS) $(CON_TARGETS) $(KERF_TARGETS)

concurrent: $(CON_TARGETS)

kerf: $(KERF_TARGETS)

toolkits/%: toolkits/%.cpp $(HEADERS)
	$(MPICXX) $(CXXFLAGS) -o $@ $< $(SYSLIBS)

concurrent/%: concurrent/%.cpp $(HEADERS)
	$(MPICXX) $(CXXFLAGS) -o $@ $< $(SYSLIBS)

kerf/%: kerf/%.cpp $(HEADERS)
	$(MPICXX) $(CXXFLAGS) -o $@ $< $(SYSLIBS)

TIME = /usr/bin/time -v -o $(PROFILE_PATH)/$(DATA)/$@.time sh -c
PERF = perf stat -d -o $(PROFILE_PATH)/$(DATA)/$@.perf sh -c

expcon: build homo1 homo2 heter mbfs msssp
build:
	-mkdir -p $(PROFILE_PATH)/$(DATA)

homo1:
	$(TIME) "./concurrent/homo1 $(DATASET) $(SIZE)"
	$(PERF) "./concurrent/homo1 $(DATASET) $(SIZE)"

homo2:
	$(TIME) "./concurrent/homo2 $(DATASETW) $(SIZE)"
	$(PERF) "./concurrent/homo2 $(DATASETW) $(SIZE)"

heter:
	$(TIME) "./concurrent/heter $(DATASETW) $(SIZE)"
	$(PERF) "./concurrent/heter $(DATASETW) $(SIZE)"

mbfs:
	$(TIME) "./concurrent/mbfs $(DATASET) $(SIZE)"
	$(PERF) "./concurrent/mbfs $(DATASET) $(SIZE)"

msssp:
	$(TIME) "./concurrent/msssp $(DATASETW) $(SIZE)"
	$(PERF) "./concurrent/msssp $(DATASETW) $(SIZE)"


expge: build homo1ge homo2ge heterge mbfsge mssspge

homo1ge:
	$(TIME) './toolkits/bfs $(DATASET) $(SIZE) 10 & \
	./toolkits/bfs $(DATASET) $(SIZE) 20 & \
	./toolkits/bfs $(DATASET) $(SIZE) 30 & \
	./toolkits/bfs $(DATASET) $(SIZE) 40 & \
	./toolkits/cc $(DATASET) $(SIZE)  & \
	./toolkits/cc $(DATASET) $(SIZE)  & \
	./toolkits/cc $(DATASET) $(SIZE)  & \
	./toolkits/cc $(DATASET) $(SIZE)  & \
	wait'

homo2ge:
	$(TIME) './toolkits/sssp $(DATASETW) $(SIZE) 73 & \
	./toolkits/sssp $(DATASETW) $(SIZE) 144 & \
	./toolkits/sssp $(DATASETW) $(SIZE) 215 & \
	./toolkits/sssp $(DATASETW) $(SIZE) 286 & \
	./toolkits/pagerank $(DATASET) $(SIZE) 100 & \
	./toolkits/pagerank $(DATASET) $(SIZE) 100 & \
	./toolkits/pagerank $(DATASET) $(SIZE) 100 & \
	./toolkits/pagerank $(DATASET) $(SIZE) 100 & \
	wait'

heterge:
	$(TIME) './toolkits/bfs $(DATASET) $(SIZE) 71 & \
	./toolkits/cc $(DATASET) $(SIZE)  & \
	./toolkits/pagerank $(DATASET) $(SIZE) 100 & \
	./toolkits/sssp $(DATASETW) $(SIZE) 102 & \
	./toolkits/bfs $(DATASET) $(SIZE) 142 & \
	./toolkits/cc $(DATASET) $(SIZE)  & \
	./toolkits/pagerank $(DATASET) $(SIZE) 100 & \
	./toolkits/sssp $(DATASETW) $(SIZE) 203 & \
	wait'

mbfsge:
	$(TIME) './toolkits/bfs $(DATASET) $(SIZE) 91 & \
	./toolkits/bfs $(DATASET) $(SIZE) 182 & \
	./toolkits/bfs $(DATASET) $(SIZE) 273 & \
	./toolkits/bfs $(DATASET) $(SIZE) 364 & \
	./toolkits/bfs $(DATASET) $(SIZE) 455 & \
	./toolkits/bfs $(DATASET) $(SIZE) 546 & \
	./toolkits/bfs $(DATASET) $(SIZE) 637 & \
	./toolkits/bfs $(DATASET) $(SIZE) 828 & \
	wait'

mssspge:
	$(TIME) './toolkits/sssp $(DATASETW) $(SIZE) 211 & \
	./toolkits/sssp $(DATASETW) $(SIZE) 422 & \
	./toolkits/sssp $(DATASETW) $(SIZE) 633 & \
	./toolkits/sssp $(DATASETW) $(SIZE) 844 & \
	./toolkits/sssp $(DATASETW) $(SIZE) 1055 & \
	./toolkits/sssp $(DATASETW) $(SIZE) 1266 & \
	./toolkits/sssp $(DATASETW) $(SIZE) 1477 & \
	./toolkits/sssp $(DATASETW) $(SIZE) 1688 & \
	wait'

gendata:
	python utils/converter.py ../Dataset/cit-Patents
	python utils/converter.py ../Dataset/cit-Patents-w
	python utils/converter.py ../Dataset/soc-LiveJournal1
	python utils/converter.py ../Dataset/soc-LiveJournal1-w
	python utils/converter.py ../Dataset/rMatGraph24
	python utils/converter.py ../Dataset/rMatGraph24-w

.PHONY: clean
clean:
	rm -f $(TARGETS)