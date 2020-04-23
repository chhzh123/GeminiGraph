// Generated by the property buffer compiler. DO NOT EDIT!
// source: M-BFS.prop

#ifndef MBFS_PROPERTY_BUFFER_H
#define MBFS_PROPERTY_BUFFER_H

#include <cctype>
#include <cstdlib>
#include <vector>
#include "parallel.h"

namespace MBFS {

typedef int intE;
typedef unsigned int uintE;

class PropertyManager;

namespace BFS_Prop {

class Parents {
public:
  Parents(size_t _n): n(_n) {
  }
  Parents(const Parents& other) {
    this->n = other.n;
    this->data = other.data;
    this->sj_num = other.sj_num;
  }
  ~Parents() {
    free(data);
  }
  inline Parents& operator=(const Parents& other) {
    this->n = other.n;
    this->data = other.data;
    this->sj_num = other.sj_num;
    return *this;
  }
  inline uintE operator[] (int i) const { return data[i * sj_num]; }
  inline uintE& operator[] (int i) { return data[i * sj_num]; }
  inline uintE get (int i) const { return data[i * sj_num]; }
  inline uintE& get (int i) { return data[i * sj_num]; }
  inline uintE* get_addr (int i) { return &(data[i * sj_num]); }
  inline uintE* get_data () { return data; }
  inline void set (int i, uintE val) { data[i * sj_num] = val; }
  inline void set_all (uintE val) { parallel_for (int i = 0; i < n; ++i) data[i * sj_num] = val; }
  inline void add (int i, uintE val) { data[i * sj_num] += val; }
  friend class MBFS::PropertyManager;
private:
  inline void set_same_job_num (int num) { sj_num = num; }
  size_t n;
  int sj_num;
  uintE* data;
};

} // namespace BFS_Prop

class PropertyMessage {
public:
  PropertyMessage(uintE val) {
    for(int i = 0; i < 8; ++i) data[i] = val;
  }
  inline uintE operator[] (int i) const { return data[i]; }
  inline uintE& operator[] (int i) { return data[i]; }
  inline uintE get (int i) const { return data[i]; }
  inline uintE& get (int i) { return data[i]; }
  inline uintE* get_data () { return data; }
  inline void set (int i, uintE val) { data[i] = val; }
private:
  uintE data[8];
};

class PropertyManager {
public:
  size_t n;
  PropertyManager(size_t _n): n(_n) {}
  inline BFS_Prop::Parents* add_Parents() {
    BFS_Prop::Parents* Parents = new BFS_Prop::Parents(n);
    arr_BFS_Parents.push_back(Parents);
    return Parents;
  }
  inline void initialize() {
    //  BFS_Prop::Parents
    int arr_BFS_Parents_size = arr_BFS_Parents.size();
    int arr_BFS_Parents_all_size = n * arr_BFS_Parents_size;
    arr_BFS_Parents_all = (uintE*) malloc(sizeof(uintE) * arr_BFS_Parents_all_size);
    parallel_for (int i = 0; i < arr_BFS_Parents_all_size; ++i) {
      arr_BFS_Parents_all[i] = UINT_MAX;
    }
    int arr_BFS_Parents_idx = 0;
    for (auto ptr : arr_BFS_Parents) {
      ptr->set_same_job_num(arr_BFS_Parents_size);
      ptr->data = &(arr_BFS_Parents_all[arr_BFS_Parents_idx]);
      arr_BFS_Parents_idx += 1;
    }
  }
  inline PropertyMessage* get_property() {
    return (PropertyMessage*) arr_BFS_Parents_all;
  }
  std::vector<BFS_Prop::Parents*> arr_BFS_Parents;
  uintE* arr_BFS_Parents_all;
};


} // namespace MBFS

#endif // MBFS_PROPERTY_BUFFER_H