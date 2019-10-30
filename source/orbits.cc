#include <vector>

#include "orbits.h"
#include "perm.h"

namespace cgtl
{

std::vector<unsigned>
orbit_of(unsigned x, std::vector<Perm> const &generators)
{
  std::vector<unsigned> res {x};

  if (generators.empty())
    return res;

  std::vector<unsigned> in_orbit(generators[0].degree() + 1, 0);
  in_orbit[x] = 1;

  std::vector<unsigned> stack {x};

  while (!stack.empty()) {
    unsigned y = stack.back();
    stack.pop_back();

    for (Perm const &gen : generators) {
      unsigned y_prime = gen[y];

      if (!in_orbit[y_prime]) {
        in_orbit[y_prime] = 1;

        stack.push_back(y_prime);
        res.push_back(y_prime);
      }
    }
  }

  return res;
}

std::vector<std::vector<unsigned>>
orbit_partition(std::vector<Perm> const &generators)
{
  unsigned n = generators[0].degree();

  std::vector<std::vector<unsigned>> res;
  std::vector<int> orbit_indices(n + 1u, -1);

  unsigned processed = 0u;

  for (auto i = 1u; i <= n; ++i) {
    int orbit_index1 = orbit_indices[i];
    if (orbit_index1 == -1) {
      orbit_index1 = static_cast<int>(res.size());
      orbit_indices[i] = orbit_index1;

      res.push_back({i});

      if (++processed == n)
        return res;
    }

    for (Perm const &gen : generators) {
      unsigned j = gen[i];

      int orbit_index2 = orbit_indices[j];
      if (orbit_index2 == -1) {
        res[orbit_index1].push_back(j);
        orbit_indices[j] = orbit_index1;

        if (++processed == n)
          return res;
      }
    }
  }

  return res;
}

} // namespace cgtl
