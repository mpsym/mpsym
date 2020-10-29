#include <algorithm>
#include <cassert>
#include <limits>
#include <memory>
#include <ostream>
#include <random>
#include <stdexcept>
#include <vector>

#include <boost/multiprecision/cpp_int.hpp>

#include "bsgs.hpp"
#include "orbits.hpp"
#include "perm.hpp"
#include "perm_group.hpp"
#include "perm_set.hpp"
#include "util.hpp"

namespace mpsym
{

namespace internal
{

PermGroup::PermGroup(unsigned degree, PermSet const &generators)
{
  _bsgs = BSGS(degree, generators);
  _order = _bsgs.order();
}

bool PermGroup::operator==(PermGroup const &rhs) const
{
  assert(rhs.degree() == degree()
         && "comparing permutation groups of equal degree");

  if (_order != rhs.order())
    return false;

  for (Perm const &gen : rhs.generators()) {
    if (!contains_element(gen))
      return false;
  }

  return true;
}

bool PermGroup::operator!=(PermGroup const &rhs) const
{
  return !(*this == rhs);
}

PermGroup PermGroup::symmetric(unsigned degree)
{
  // TODO: explicit BSGS

  assert(degree > 0u);

  if (degree == 1u)
    return PermGroup(1u, {Perm(1u)});

  std::vector<unsigned> gen;
  for (unsigned i = 1u; i <= degree; ++i)
    gen.push_back(i);

  return PermGroup(degree, {Perm(degree, {{1, 2}}), Perm(degree, {gen})});
}

PermGroup PermGroup::cyclic(unsigned degree)
{
  // TODO: explicit BSGS

  assert(degree > 0u);

  std::vector<unsigned> gen;
  for (unsigned i = 1u; i <= degree; ++i)
    gen.push_back(i);

  return PermGroup(degree, {Perm(degree, {gen})});
}

PermGroup PermGroup::alternating(unsigned degree)
{
  // TODO: explicit BSGS

  assert(degree > 0u);

  if (degree == 1u)
    return PermGroup(1, {});

  if (degree == 2u)
    return PermGroup(2, {});

  PermSet gens;
  for (unsigned i = 3u; i <= degree; ++i)
    gens.emplace(Perm(degree, {{1, 2, i}}));

  return PermGroup(degree, gens);
}

PermGroup PermGroup::dihedral(unsigned degree)
{
  // TODO: explicit BSGS

  assert(degree > 0u && degree % 2 == 0);

  if (degree == 2u)
    return PermGroup(2, {Perm({2, 1})});

  if (degree == 4u)
    return PermGroup(4, {Perm({2, 1, 3, 4}), Perm({1, 2, 4, 3})});

  std::vector<unsigned> rotation(degree / 2u);

  // rotation
  for (unsigned i = 0u; i < degree / 2u - 1u; ++i)
    rotation[i] = i + 2u;

  rotation[degree / 2u - 1u] = 1u;

  // reflection
  std::vector<unsigned> reflection(degree / 2u);

  reflection[0] = 1u;

  for (unsigned i = 1u; i < (degree / 2u + 1u) / 2u; ++i) {
    reflection[i] = degree / 2u - i + 1u;
    reflection[degree / 2u - i] = i + 1u;
  }

  if ((degree / 2u) % 2 == 0)
    reflection[degree / 4u] = degree / 4u + 1u;

  return PermGroup(degree / 2u, {Perm(rotation), Perm(reflection)});
}

PermGroup PermGroup::wreath_product(PermGroup const &lhs_,
                                    PermGroup const &rhs_,
                                    BSGSOptions const *bsgs_options_)
{
  if (lhs_.is_trivial())
    return rhs_;

  if (rhs_.is_trivial())
    return lhs_;

  auto lhs(lhs_.generators());
  auto rhs(rhs_.generators());

  lhs.assert_not_empty();
  rhs.assert_not_empty();

  lhs.minimize_degree();
  rhs.minimize_degree();

  // degree of wreath product
  unsigned wp_degree = lhs.degree() * rhs.degree();

  // order of wreath product
  auto wp_order(wreath_product_order(lhs_, rhs_));

  // determine generators of wreath product
  PermSet wp_generators;

  for (unsigned i = 0u; i < rhs.degree(); ++i) {
    for (Perm const &perm : lhs)
      wp_generators.insert(perm.shifted(lhs.degree() * i).extended(wp_degree));
  }

  for (Perm const &gen_rhs : rhs) {
    std::vector<std::vector<unsigned>> cycles {gen_rhs.cycles()};
    for (auto &cycle : cycles) {
      for (unsigned &x : cycle)
        x = (x - 1u) * lhs.degree() + 1u;
    }

    std::vector<std::vector<unsigned>> shifted_cycles {cycles};

    for (unsigned i = 1u; i < lhs.degree(); ++i) {
      for (auto const &cycle : cycles) {
        std::vector<unsigned> shifted_cycle(cycle);

        for (unsigned &x : shifted_cycle)
          x += i;

        shifted_cycles.push_back(shifted_cycle);
      }
    }

    wp_generators.emplace(wp_degree, shifted_cycles);
  }

  // construct wreath product
  auto bsgs_options(BSGSOptions::fill_defaults(bsgs_options_));
  bsgs_options.schreier_sims_random_known_order = wp_order;

  return PermGroup(BSGS(wp_degree, wp_generators, &bsgs_options));
}

BSGS::order_type PermGroup::wreath_product_order(PermGroup const &lhs,
                                                 PermGroup const &rhs)
{
  using boost::multiprecision::pow;

  auto lhs_order(lhs.order());
  auto rhs_order(rhs.order());

  if (lhs.is_trivial())
    return rhs_order;

  if (rhs.is_trivial())
    return lhs_order;

  unsigned rhs_lmp = rhs.largest_moved_point();

  return pow(lhs_order, rhs_lmp) * rhs_order;
}

bool PermGroup::is_symmetric() const
{
  if (_bsgs.is_symmetric() || degree() == 1u)
    return true;

  return _order == symmetric_order(degree());
}

bool PermGroup::is_shifted_symmetric() const
{
  unsigned degree_ = largest_moved_point() - smallest_moved_point() + 1u;

  return _order == symmetric_order(degree_);
}

bool PermGroup::is_alternating() const
{
  if (_bsgs.is_alternating())
    return true;

  if (degree() == 1u)
    return false;

  if (degree() == 2u)
    return _order == 1u;

  return _order == alternating_order(degree());
}

bool PermGroup::is_shifted_alternating() const
{
  unsigned degree_ = largest_moved_point() - smallest_moved_point() + 1u;

  return _order == alternating_order(degree_);
}

bool PermGroup::is_transitive() const
{
  auto orbit(Orbit::generate(1u, generators().with_inverses()));

  return orbit.size() == degree();
}

bool PermGroup::contains_element(Perm const &perm) const
{
  assert(perm.degree() == degree() && "element has same degree as group");

  return _bsgs.strips_completely(perm);
}

Perm PermGroup::random_element() const
{
  static auto re(util::random_engine());

  Perm result(degree());
  for (unsigned i = 0u; i < _bsgs.base_size(); ++i) {
    std::vector<unsigned> orbit = _bsgs.orbit(i);
    std::uniform_int_distribution<> d(0u, orbit.size() - 1u);
    result *= _bsgs.transversal(i, orbit[d(re)]);
  }

  return result;
}

PermGroup::const_iterator::const_iterator(PermGroup const &pg)
  : _trivial(pg.bsgs().base_empty()),
    _end(false)
{
  if (_trivial) {
    _current = Perm(pg.degree());

    _current_valid = true;

  } else {
    for (unsigned i = 0u; i < pg.bsgs().base_size(); ++i) {
      _state.push_back(0u);

      auto transv = pg.bsgs().transversals(i);

      _transversals.push_back(transv);
      _current_factors.insert(transv[0]);
    }

    _current_valid = false;
  }
}

bool PermGroup::const_iterator::operator==(PermGroup::const_iterator const &rhs) const
{
  if (_end != rhs._end)
    return false;

  if (_end && rhs._end)
    return true;

  for (unsigned i = 0u; i < _state.size(); ++i) {
    if (_state[i] != rhs._state[i])
      return false;
  }

  return true;
}

PermGroup::const_iterator::reference PermGroup::const_iterator::current()
{
  if (_current_valid)
    return _current;

  _current = _current_factors[0];
  for (unsigned j = 1u; j < _current_factors.size(); ++j)
    _current = _current_factors[j] * _current;

  _current_valid = true;

  return _current;
}

void PermGroup::const_iterator::next()
{
  if (_trivial) {
    _end = true;
    return;
  }

  for (unsigned i = 0u; i < _state.size(); ++i) {
    _state[i]++;
    if (_state[i] == _transversals[i].size())
      _state[i] = 0u;

    _current_factors[i] = _transversals[i][_state[i]];

    if (i == _state.size() - 1u && _state[i] == 0u) {
      _end = true;
      break;
    }

    if (_state[i] != 0u)
      break;
  }

  _current_valid = false;
}

std::ostream &operator<<(std::ostream &os, PermGroup const &pg)
{
  os << pg.bsgs() << "\n"
     << "ORDER: " << pg._order;

  return os;
}

} // namespace internal

} // namespace mpsym
