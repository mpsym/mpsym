#ifndef GUARD_ARCH_GRAPH_H
#define GUARD_ARCH_GRAPH_H

#include <memory>
#include <ostream>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include <boost/graph/adjacency_list.hpp>

#include "arch_graph_system.hpp"
#include "nauty_graph.hpp"
#include "perm_group.hpp"

namespace mpsym
{

namespace internal { class NautyGraph; }

class ArchGraph : public ArchGraphSystem
{
  friend std::ostream &operator<<(std::ostream &os, ArchGraph &ag);

  typedef std::vector<std::string>::size_type processor_type_size_type;
  typedef std::vector<std::string>::size_type channel_type_size_type;

  template<typename L>
  using typed_channel_dict_type = std::unordered_map<
    unsigned,
    std::vector<std::pair<unsigned, L>>
  >;

  using untyped_channel_dict_type = std::unordered_map<
    unsigned,
    std::vector<unsigned>
  >;

  typedef boost::vecS vertex_selector;
  typedef boost::vecS edge_selector;

  struct VertexProperty { processor_type_size_type type; };
  struct EdgeProperty { channel_type_size_type type; };

  typedef boost::adjacency_list<
    edge_selector, vertex_selector, boost::directedS,
    VertexProperty, EdgeProperty> adjacency_type;

  typedef adjacency_type::vertices_size_type vertices_size_type;
  typedef adjacency_type::edges_size_type edges_size_type;

public:
  typedef processor_type_size_type ProcessorType;
  typedef channel_type_size_type ChannelType;

  template<typename L = void>
  using ChannelDictType = typename std::conditional<
    std::is_void<L>::value,
    untyped_channel_dict_type,
    typed_channel_dict_type<L>
  >::type;

  ArchGraph(bool directed = false)
  : _directed(directed)
  {}

  virtual ~ArchGraph() = default;

  std::string to_gap() const override;
  std::string to_json() const override;

  ProcessorType new_processor_type(std::string const &pl);
  ChannelType new_channel_type(std::string const &cl);

  unsigned add_processor(ProcessorType pt);
  unsigned add_processor(std::string const &pl);

  template<typename T>
  unsigned add_processors(unsigned n, T pt)
  {
    assert(n > 0u);

    for (unsigned i = 0u; i < n - 1u; ++i)
      add_processor(pt);

    return add_processor(pt);
  }

  void add_channel(unsigned pe1, unsigned pe2, ChannelType ct);
  void add_channel(unsigned pe1, unsigned pe2, std::string const &cl);

  template<typename T>
  void add_channels(ChannelDictType<T> const &channels)
  {
    for (auto const &tmp : channels) {
      unsigned pe1 = tmp.first;
      auto const &to = tmp.second;

      for (auto const &c : to) {
        unsigned pe2 = c.first;
        T ct = c.second;

        add_channel(pe1, pe2, ct);
      }
    }
  }

  template<typename T>
  void add_channels(ChannelDictType<> const &channels, T ct)
  {
    for (auto const &tmp : channels) {
      unsigned pe1 = tmp.first;
      auto const &to = tmp.second;

      for (auto &pe2 : to)
        add_channel(pe1, pe2, ct);
    }
  }

  template<typename T>
  void fully_connect(T ct)
  {
    for (unsigned pe1 = 0u; pe1 < num_processors(); ++pe1) {
      for (unsigned pe2 = (directed() ? 0u : pe1); pe2 < num_processors(); ++pe2)
        add_channel(pe1, pe2, ct);
    }
  }

  void fully_connect(ProcessorType pt, ChannelType ct);
  void fully_connect(std::string const &pl, std::string const &cl);

  template<typename T>
  void self_connect(T ct)
  {
    for (unsigned pe = 0u; pe < num_processors(); ++pe)
      add_channel(pe, pe, ct);
  }

  void self_connect(ProcessorType pt, ChannelType ct);
  void self_connect(std::string const &pl, std::string const &cl);

  bool directed() const;
  bool effectively_directed() const;

  unsigned num_processors() const override;
  unsigned num_channels() const override;

private:
  internal::PermGroup automorphisms_(
    AutomorphismOptions const *options) override
  { return automorphisms_nauty(options); }

  ChannelType assert_channel_type(std::string const &cl);
  ProcessorType assert_processor_type(std::string const &cl);

  void add_self_channel(unsigned pe, ChannelType ct);
  void add_non_self_channel(unsigned from, unsigned to, ChannelType ct);
  static std::string add_self_channel_to_processor_label(std::string const &pl,
                                                         std::string const &cl);

  bool channel_exists(unsigned from, unsigned to, ChannelType ct) const;
  bool channel_exists_directed(unsigned from, unsigned to, ChannelType ct) const;
  bool channel_exists_undirected(unsigned from, unsigned to, ChannelType ct) const;

  internal::NautyGraph graph_nauty() const;
  std::string to_gap_nauty() const;
  internal::PermGroup automorphisms_nauty(AutomorphismOptions const *options) const;

  void dump_processors(std::ostream& os) const;
  void dump_channels(std::ostream& os) const;
  void dump_automorphisms(std::ostream& os);

  adjacency_type _adj;
  bool _directed;

  std::vector<std::string> _processor_types;
  std::vector<std::string> _channel_types;

  std::vector<vertices_size_type> _processor_type_instances;
  std::vector<edges_size_type> _channel_type_instances;
};

std::ostream &operator<<(std::ostream &os, ArchGraph &ag);

}

#endif // GUARD_ARCH_GRAPH_H
