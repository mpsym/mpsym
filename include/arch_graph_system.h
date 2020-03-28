#ifndef _GUARD_ARCH_GRAPH_SYSTEM_H
#define _GUARD_ARCH_GRAPH_SYSTEM_H

#include <memory>
#include <string>

#include "bsgs.h"
#include "partial_perm_inverse_semigroup.h"
#include "perm_group.h"
#include "task_allocation.h"
#include "task_orbits.h"

namespace mpsym
{

class ArchGraphSystem
{
public:
  enum class ReprMethod {
    ITERATE,
    LOCAL_SEARCH,
    ORBITS,
    AUTO = ITERATE
  };

  struct ReprOptions {
    ReprMethod method = ReprMethod::AUTO;
    unsigned offset = 0u;
    bool match_reprs = true;
  };

  static std::shared_ptr<ArchGraphSystem> from_lua(std::string const &lua);

  virtual std::string to_gap() const = 0;

  virtual unsigned num_processors() const
  { throw std::logic_error("not implemented"); }

  virtual unsigned num_channels() const
  { throw std::logic_error("not implemented"); }

  virtual PermGroup automorphisms(BSGS::Options const *bsgs_options = nullptr)
  {
    if (!_automorphisms_valid) {
      _automorphisms = update_automorphisms(bsgs_options);
      _automorphisms_valid = true;
    }

    return _automorphisms;
  }

  void invalidate_automorphisms()
  { _automorphisms_valid = false; }

  virtual TaskAllocation repr(TaskAllocation const &allocation,
                              ReprOptions const *options = nullptr,
                              TaskOrbits *orbits = nullptr);

protected:
  static ReprOptions complete_options(ReprOptions const *options)
  {
    static ReprOptions _default_mapping_options;

    return options ? *options : _default_mapping_options;
  }

private:
  virtual PermGroup update_automorphisms(BSGS::Options const *bsgs_options) = 0;

  TaskAllocation min_elem_iterate(TaskAllocation const &tasks,
                                  ReprOptions const *options,
                                  TaskOrbits *orbits);

  TaskAllocation min_elem_local_search(TaskAllocation const &tasks,
                                       ReprOptions const *options,
                                       TaskOrbits *orbits);

  TaskAllocation min_elem_orbits(TaskAllocation const &tasks,
                                 ReprOptions const *options,
                                 TaskOrbits *orbits);

  static bool is_repr(TaskAllocation const &tasks,
                                ReprOptions const *options,
                                TaskOrbits *orbits)
  {
    if (!options->match_reprs || !orbits)
      return false;

    return orbits->is_repr(tasks);
  }

  PermGroup _automorphisms;
  bool _automorphisms_valid = false;
};

// TODO: outstream operator

} // namespace mpsym

#endif // _GUARD_ARCH_GRAPH_SYSTEM_H
