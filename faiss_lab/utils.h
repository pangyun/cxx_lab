#pragma once

#include <iostream>
#include <string>
#include <chrono>
#include <faiss/index_io.h>
#include <faiss/IndexFlat.h>
#include <faiss/gpu/StandardGpuResources.h>
#include <faiss/gpu/GpuIndex.h>
#include <faiss/gpu/GpuIndexFlat.h>
#include <faiss/utils/utils.h>
#include <faiss/AutoTune.h>
#include <faiss/gpu/GpuAutoTune.h>
#include <faiss/IndexIVF.h>
#include <faiss/gpu/GpuIndexIVF.h>
#include <faiss/index_factory.h>
#include <faiss/gpu/GpuClonerOptions.h>
#include <faiss/gpu/GpuCloner.h>
#include <faiss/utils/distances.h>
#include <thread>
#include <sstream>

using namespace std;

#if 1
#define TIMING

#ifdef TIMING
#define INIT_TIMER auto start_time = std::chrono::high_resolution_clock::now();
                   /* auto MSG_FUNC = [&](const string& msg) -> string {return msg;}; */
#define START_TIMER  start_time = std::chrono::high_resolution_clock::now();
#define STOP_TIMER_WITH_FUNC(name)  cout << "RUNTIME of " << MSG_FUNC(name) << ": " << \
    std::chrono::duration_cast<std::chrono::milliseconds>( \
            std::chrono::high_resolution_clock::now()-start_time \
    ).count() << " ms " << endl;
#define STOP_TIMER(name)  cout << "RUNTIME of " << name << ": " << \
    std::chrono::duration_cast<std::chrono::milliseconds>( \
            std::chrono::high_resolution_clock::now()-start_time \
    ).count() << " ms " << endl;
#else
#define INIT_TIMER
#define START_TIMER
#define STOP_TIMER(name)
#endif
#endif

namespace faiss {
    class Index;
    namespace gpu {
        class GpuResources;
    }
}


struct TestData {
    TestData(int dimension, long size);

    ~TestData();

    float* xb;
    long nb;
    int d;
};

struct TestFactory {
    int search_times = 1;
    int gpu_num = 0;
    int d = 512;
    int nb = 0;
    int nq = 1;
    int k = 1024;
    int nprobe = 64;
    bool verbose = false;
    bool useFloat16 = false;
    bool readonly = true;
    string output = "";
    string input = "";
    string index_type = "IVF16384,Flat";
    shared_ptr<TestData> data = nullptr;
    shared_ptr<faiss::Index> index = nullptr;

    TestFactory(const string& fname = "") : input(fname) {
        if (fname == "") return;
        index.reset(faiss::read_index(input.c_str()));
        d = index->d;
        nb = index->ntotal;
        cout << "\033[1;32m" << input << "\033[0m ----------->" << endl;
        cout << "\033[1;31m" << IndexInfo() << "\033[0m" << endl;
    }

    ~TestFactory() {
        cout << "\033[1;31m" << IndexInfo() << "\033[0m" << endl;
        Serialize();
    }

    void Serialize() {
        if(!index || output == "") {
            return;
        }

        faiss::write_index(index.get(), output.c_str());
        cout << "-------> " << "\033[1;32m" << output << "\033[0m" << endl;
    }

    bool IsReadOnlyIndex() {
        auto ivf = dynamic_pointer_cast<faiss::IndexIVF>(index);
        if (!ivf) {
            return false;
        }
        return ivf->is_readonly();
    }

    std::string IndexInfo() const {
        auto name = typeid(*(index.get())).name();
        auto ivf = dynamic_pointer_cast<faiss::IndexIVF>(index);
        stringstream ss;
        ss << name;
        if (ivf) {
            ss << ",IVF" << ivf->invlists->nlist;
            ss << "," << (ivf->is_readonly()?"RO":"WR");
            ss << ",NP" << ivf->nprobe;
        }
        ss << ",D" << index->d;
        ss << ",N" << index->ntotal;
        return ss.str();
    }

    void MakeData() {
        if(data) return;
        data = make_shared<TestData>(d, nb);
    }

    void MakeIndex(bool force = false) {
        if (index) {
            index->verbose = verbose;
        }
        if (index && !force) return;
        auto MSG_FUNC = [&](const string& msg) -> string {
            stringstream ss;
            ss << index_type << "_" << gpu_num << "_" << msg;
            return ss.str();
        };

        INIT_TIMER;

        START_TIMER;
        MakeData();
        STOP_TIMER_WITH_FUNC("MakeData");

        faiss::gpu::StandardGpuResources gpu_res;
        auto cpu_index = faiss::index_factory(d, index_type.c_str());
        /* faiss::gpu::GpuClonerOptions clone_option; */
        /* clone_option.useFloat16 = true; */
        auto gpu_index = faiss::gpu::index_cpu_to_gpu(&gpu_res, gpu_num, cpu_index);
        /* auto gpu_index = faiss::gpu::index_cpu_to_gpu(&gpu_res, gpu_num, cpu_index, &clone_option); */

        delete cpu_index;

        START_TIMER;
        gpu_index->train(data->nb, data->xb);
        STOP_TIMER_WITH_FUNC("Train");

        START_TIMER;
        gpu_index->add(data->nb, data->xb);
        STOP_TIMER_WITH_FUNC("AddXB");

        START_TIMER;
        cpu_index = faiss::gpu::index_gpu_to_cpu(gpu_index);
        STOP_TIMER_WITH_FUNC("GpuToCpu");
        delete gpu_index;

        if (readonly) {
            auto ivf = dynamic_cast<faiss::IndexIVF*>(cpu_index);
            if (ivf) {
                ivf->to_readonly();
            }
        }

        index.reset(cpu_index);
        index->verbose = verbose;
    }
};



void search_index_test(faiss::Index* index, const string& context, int nq, int k,
        long nb, float *xb, int times, bool do_print = false);
void gpu_add_vectors_test(faiss::gpu::GpuResources* gpu_res, const string& context, int times,
        long start, long end, long step, float* xb, int d);
void cpu_to_gpu_test(faiss::gpu::GpuResources* gpu_res, int gpu_num, faiss::Index* index, const string& context, int times);

void quantizer_cloner_test();

void ivf_sq_test();

void gpu_ivf_sq_test();

void inverted_list_test(TestFactory&);
