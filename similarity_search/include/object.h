/**
 * Non-metric Space Library
 *
 * Authors: Bilegsaikhan Naidan (https://github.com/bileg), Leonid Boytsov (http://boytsov.info).
 * With contributions from Lawrence Cayton (http://lcayton.com/).
 *
 * For the complete list of contributors and further details see:
 * https://github.com/searchivarius/NonMetricSpaceLib 
 * 
 * Copyright (c) 2010--2013
 *
 * This code is released under the
 * Apache License Version 2.0 http://www.apache.org/licenses/.
 *
 */

#ifndef _OBJECT_H_
#define _OBJECT_H_

#include <string.h>

#include <vector>
#include <list>
#include <utility>

#include "global.h"
#include "logging.h"

namespace similarity {

// Structure of object: | id | datasize | data ........ |

typedef int IdType;

const size_t ID_SIZE = sizeof(IdType);
const size_t DATALENGTH_SIZE = sizeof(size_t);

class Object {
 public:
  explicit Object(char* buffer) : buffer_(buffer), memory_allocated_(false) {}

  Object(IdType id, size_t datalength, const void* data) {
    buffer_ = new char[ID_SIZE + DATALENGTH_SIZE + datalength];
    CHECK(buffer_ != NULL);
    memory_allocated_ = true;
    char* ptr = buffer_;
    memcpy(ptr, &id, ID_SIZE);
    ptr += ID_SIZE;
    memcpy(ptr, &datalength, DATALENGTH_SIZE);
    ptr += DATALENGTH_SIZE;
    if (data != NULL) {
      memcpy(ptr, data, datalength);
    } else {
      memset(ptr, 0, datalength);
    }
  }

  ~Object() {
    if (memory_allocated_) {
      delete[] buffer_;
    }
  }

  enum { kDummyId = -1 };

  static Object* CreateNewEmptyObject(size_t datalength) {
    // the caller is responsible for releasing the pointer
    Object* empty_object = new Object(kDummyId, datalength, NULL);
    CHECK(empty_object != NULL);
    return empty_object;
  }

  Object* Clone() const {
    Object* clone = new Object(id(), datalength(), data());
    return clone;
  }

  inline IdType id() const { return *(reinterpret_cast<int*>(buffer_)); }
  inline const char* data() const { return buffer_ + ID_SIZE + DATALENGTH_SIZE; }
  inline char* data()             { return buffer_ + ID_SIZE + DATALENGTH_SIZE; }
  inline size_t datalength() const { return *(reinterpret_cast<size_t*>(buffer_ + ID_SIZE));}
  inline const char* buffer() const { return buffer_; }
  inline size_t bufferlength() const { return ID_SIZE + DATALENGTH_SIZE + datalength(); }

  void Print() const {
    LOG(INFO) << "id = " << id()
        << "\tdatalength = " << datalength()
        << "\tbuffer = " << buffer()   // %p
        << "\tdata = " << data();  // %p
  }

 private:
  char* buffer_;
  bool memory_allocated_;

  // disable copy and assign
  DISABLE_COPY_AND_ASSIGN(Object);
};


typedef std::vector<const Object*> ObjectVector;

/* 
 * The caller is repsonsible for deleting:
 *  1) bucket
 *  2) Object pointers stored in the bucket
 */
inline void CreateCacheOptimizedBucket(const ObjectVector& data, 
                                       char*& CacheOptimizedBucket, 
                                       ObjectVector*& bucket) {
  CHECK(data.size());
  CacheOptimizedBucket = new char [data.size() * data[0]->bufferlength()];
  char *p = CacheOptimizedBucket;
  bucket = new ObjectVector(data.size());

  for(size_t i = 0; i < data.size(); ++i) {
    memcpy(p, data[i]->buffer(), data[i]->bufferlength());
    (*bucket)[i] = new Object(const_cast<char*>(p));
    p += data[i]->bufferlength();
  }
}

inline void ClearBucket(char* CacheOptimizedBucket,
                        ObjectVector* bucket) {
  if (CacheOptimizedBucket) {
    for(auto i:(*bucket)) {
      delete i;
    }
  }
  delete [] CacheOptimizedBucket;
  delete bucket;
}

typedef std::list<const Object*> ObjectList;

template<typename dist_t>
using DistObjectPair = std::pair<dist_t, const Object*>;

template <typename dist_t>
using DistObjectPairVector = std::vector<DistObjectPair<dist_t>>;


template <typename dist_t>
struct DistObjectPairAscComparator {
  bool operator()(const DistObjectPair<dist_t>& x,
                  const DistObjectPair<dist_t>& y) const {
    return x.first < y.first;
  }
};

template <typename dist_t>
struct DistObjectPairDescComparator {
  bool operator()(const DistObjectPair<dist_t>& x,
                  const DistObjectPair<dist_t>& y) const {
    return x.first > y.first;
  }
};

struct ObjectIdAscComparator {
  inline bool operator()(const Object* x, const Object* y) const {
    return x->id() < y->id();
  }
};

}   // namespace similarity

#endif    // _OBJECT_H_
