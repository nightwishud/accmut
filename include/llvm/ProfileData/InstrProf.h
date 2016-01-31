//=-- InstrProf.h - Instrumented profiling format support ---------*- C++ -*-=//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Instrumentation-based profiling data is generated by instrumented
// binaries through library functions in compiler-rt, and read by the clang
// frontend to feed PGO.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_PROFILEDATA_INSTRPROF_H_
#define LLVM_PROFILEDATA_INSTRPROF_H_

#include "llvm/ADT/StringSet.h"
#include "llvm/Support/ErrorHandling.h"
#include <cstdint>
#include <list>
#include <system_error>
#include <vector>

namespace llvm {
const std::error_category &instrprof_category();

enum class instrprof_error {
  success = 0,
  eof,
  bad_magic,
  bad_header,
  unsupported_version,
  unsupported_hash_type,
  too_large,
  truncated,
  malformed,
  unknown_function,
  hash_mismatch,
  count_mismatch,
  counter_overflow,
  value_site_count_mismatch
};

inline std::error_code make_error_code(instrprof_error E) {
  return std::error_code(static_cast<int>(E), instrprof_category());
}

enum InstrProfValueKind : uint32_t {
  IPVK_IndirectCallTarget = 0,

  IPVK_First = IPVK_IndirectCallTarget,
  IPVK_Last = IPVK_IndirectCallTarget
};

struct InstrProfStringTable {
  // Set of string values in profiling data.
  StringSet<> StringValueSet;
  InstrProfStringTable() { StringValueSet.clear(); }
  // Get a pointer to internal storage of a string in set
  const char *getStringData(StringRef Str) {
    auto Result = StringValueSet.find(Str);
    return (Result == StringValueSet.end()) ? nullptr : Result->first().data();
  }
  // Insert a string to StringTable
  const char *insertString(StringRef Str) {
    auto Result = StringValueSet.insert(Str);
    return Result.first->first().data();
  }
};

struct InstrProfValueSiteRecord {
  /// Typedef for a single TargetValue-NumTaken pair.
  typedef std::pair<uint64_t, uint64_t> ValueDataPair;
  /// Value profiling data pairs at a given value site.
  std::list<ValueDataPair> ValueData;

  InstrProfValueSiteRecord() { ValueData.clear(); }

  /// Sort ValueData ascending by TargetValue
  void sortByTargetValues() {
    ValueData.sort([](const ValueDataPair &left, const ValueDataPair &right) {
      return left.first < right.first;
    });
  }

  /// Merge data from another InstrProfValueSiteRecord
  void mergeValueData(InstrProfValueSiteRecord &Input) {
    this->sortByTargetValues();
    Input.sortByTargetValues();
    auto I = ValueData.begin();
    auto IE = ValueData.end();
    for (auto J = Input.ValueData.begin(), JE = Input.ValueData.end(); J != JE;
         ++J) {
      while (I != IE && I->first < J->first)
        ++I;
      if (I != IE && I->first == J->first) {
        I->second += J->second;
        ++I;
        continue;
      }
      ValueData.insert(I, *J);
    }
  }
};

/// Profiling information for a single function.
struct InstrProfRecord {
  InstrProfRecord() {}
  InstrProfRecord(StringRef Name, uint64_t Hash, std::vector<uint64_t> Counts)
      : Name(Name), Hash(Hash), Counts(std::move(Counts)) {}
  StringRef Name;
  uint64_t Hash;
  std::vector<uint64_t> Counts;
  std::vector<InstrProfValueSiteRecord> IndirectCallSites;

  const std::vector<InstrProfValueSiteRecord> &
  getValueSitesForKind(uint32_t ValueKind) const {
    switch (ValueKind) {
    case IPVK_IndirectCallTarget:
      return IndirectCallSites;
    }
    llvm_unreachable("Unknown value kind!");
  }

  std::vector<InstrProfValueSiteRecord> &
  getValueSitesForKind(uint32_t ValueKind) {
    return const_cast<std::vector<InstrProfValueSiteRecord> &>(
        const_cast<const InstrProfRecord *>(this)
            ->getValueSitesForKind(ValueKind));
  }
};

} // end namespace llvm

namespace std {
template <>
struct is_error_code_enum<llvm::instrprof_error> : std::true_type {};
}

#endif // LLVM_PROFILEDATA_INSTRPROF_H_
