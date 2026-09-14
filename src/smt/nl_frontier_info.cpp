/******************************************************************************
 * This file is part of the cvc5 project.
 *
 * Copyright (c) 2009-2026 by the authors listed in the file AUTHORS
 * in the top-level source directory and their institutional affiliations.
 * All rights reserved.  See the file COPYING in the top-level source
 * directory for licensing information.
 * ****************************************************************************
 *
 * The (get-info :nl-frontier) reply.
 */

#include "smt/nl_frontier_info.h"

#include <algorithm>
#include <map>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "expr/node_algorithm.h"
#include "expr/skolem_manager.h"
#include "printer/smt2/smt2_printer.h"
#include "smt/assertions.h"
#include "theory/arith/nl/nl_frontier.h"
#include "theory/arith/nl/nonlinear_extension.h"
#include "theory/arith/theory_arith.h"
#include "theory/quantifiers_engine.h"
#include "theory/theory_engine.h"
#include "util/smt2_quote_string.h"

using namespace cvc5::internal::theory::arith::nl;

namespace cvc5::internal {
namespace smt {

namespace {

/** Atoms reported; the rest are counted in :omitted. */
constexpr size_t kMaxAtoms = 32;
/** Hosts reported per atom, in the input and in instantiations each. */
constexpr size_t kMaxHosts = 8;
/**
 * Instantiated terms examined in total before the instantiation search stops
 * (the reply then says :truncated true).
 */
constexpr size_t kMaxInstanceWork = 4000000;

bool isDivisionKind(Kind k)
{
  return k == Kind::INTS_DIVISION || k == Kind::INTS_DIVISION_TOTAL
         || k == Kind::INTS_MODULUS || k == Kind::INTS_MODULUS_TOTAL
         || k == Kind::DIVISION || k == Kind::DIVISION_TOTAL;
}

/** The partial operator the input wrote, for total division or modulus k. */
Kind partialKind(Kind k)
{
  switch (k)
  {
    case Kind::INTS_DIVISION_TOTAL: return Kind::INTS_DIVISION;
    case Kind::INTS_MODULUS_TOTAL: return Kind::INTS_MODULUS;
    case Kind::DIVISION_TOTAL: return Kind::DIVISION;
    default: return k;
  }
}

bool isZero(TNode n)
{
  return (n.getKind() == Kind::CONST_RATIONAL
          || n.getKind() == Kind::CONST_INTEGER)
         && n.getConst<Rational>().isZero();
}

/**
 * Whether t is what operator elimination writes for a partial division or
 * modulus by d: (ite (= d 0) (z n) (div_total n d)), z the skolem function
 * for division by zero.
 */
bool isGuardedDivision(TNode t)
{
  if (t.getKind() != Kind::ITE)
  {
    return false;
  }
  TNode d = t[2];
  Kind dk = d.getKind();
  if (dk != Kind::INTS_DIVISION_TOTAL && dk != Kind::INTS_MODULUS_TOTAL
      && dk != Kind::DIVISION_TOTAL)
  {
    return false;
  }
  TNode c = t[0];
  if (c.getKind() != Kind::EQUAL
      || !((c[0] == d[1] && isZero(c[1])) || (c[1] == d[1] && isZero(c[0]))))
  {
    return false;
  }
  TNode z = t[1];
  return z.getKind() == Kind::APPLY_UF
         && z.getOperator().getKind() == Kind::SKOLEM && z.getNumChildren() == 1
         && z[0] == d[0];
}

/**
 * The operation a term of kind k applies, named by the kind of the atoms it
 * can host (NONLINEAR_MULT for products), or UNDEFINED_KIND if none.
 */
Kind operationOf(Kind k)
{
  switch (k)
  {
    case Kind::MULT:
    case Kind::NONLINEAR_MULT: return Kind::NONLINEAR_MULT;
    case Kind::IAND:
    case Kind::PIAND: return Kind::IAND;
    case Kind::POW2:
    case Kind::EXPONENTIAL:
    case Kind::SINE:
    case Kind::INTS_DIVISION:
    case Kind::INTS_MODULUS:
    case Kind::DIVISION: return k;
    default: return Kind::UNDEFINED_KIND;
  }
}

/** Whether the arguments of operation op commute. */
bool commutes(Kind op)
{
  return op == Kind::NONLINEAR_MULT || op == Kind::IAND;
}

size_t combine(size_t h, size_t v)
{
  return h ^ (v + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2));
}

/** A fold that gives the same result whatever order the values arrive in. */
size_t combineUnordered(size_t h, size_t v)
{
  return h + v * 0x9e3779b97f4a7c15ULL;
}

/**
 * Whether the rewriter is free to reorder the children of a term of kind k.
 * Two spellings of such a term, (+ b 1) as the input wrote it and (+ 1 b) as
 * the solver holds it, are the same term, so they must compare and hash the
 * same or a host is missed.
 */
bool commutativeChildren(Kind k)
{
  return k == Kind::ADD || k == Kind::MULT || k == Kind::NONLINEAR_MULT;
}

/**
 * Terms as the input spelled them, read off the solver's terms without
 * building any: skolems read as the terms they purify, bound variables as
 * the terms bound to them, total division and modulus as the partial
 * operators the input used, and operator elimination's guarded division as
 * that division. Creating nodes here would shift the ids of the nodes the
 * next check-sat creates, and the search orders by id.
 */
class SourceView
{
 public:
  /** Read vars as vals from now on (an instantiation). */
  void bind(const std::vector<Node>& vars, const std::vector<Node>& vals)
  {
    d_bind.clear();
    for (size_t i = 0, n = vars.size(); i < n; i++)
    {
      d_bind[vars[i]] = vals[i];
    }
  }
  void unbind() { d_bind.clear(); }

  /** The term n stands for. */
  TNode top(TNode n) const
  {
    while (true)
    {
      auto it = d_bind.find(n);
      if (it != d_bind.end())
      {
        n = it->second;
        continue;
      }
      if (n.getKind() == Kind::SKOLEM)
      {
        // held by the skolem's attribute, so it outlives this TNode
        Node u = SkolemManager::getUnpurifiedForm(n);
        if (u != n)
        {
          n = u;
          continue;
        }
      }
      return n;
    }
  }
  /** The kind the input wrote, for a term t returned by top. */
  Kind kind(TNode t) const
  {
    Kind k = partialKind(isGuardedDivision(t) ? t[2].getKind() : t.getKind());
    // arithmetic rebuilds a product the input wrote as MULT, and the two
    // spellings must compare the same or a host is missed
    return k == Kind::NONLINEAR_MULT ? Kind::MULT : k;
  }
  bool isLeaf(TNode t) const
  {
    return t.getNumChildren() == 0 || t.isClosure();
  }
  size_t numChildren(TNode t) const
  {
    return isLeaf(t) ? 0 : (isGuardedDivision(t) ? 2 : t.getNumChildren());
  }
  TNode child(TNode t, size_t i) const
  {
    return isGuardedDivision(t) ? t[2][i] : t[i];
  }
  bool hasOperator(TNode t) const
  {
    return t.getMetaKind() == kind::metakind::PARAMETERIZED
           && !isGuardedDivision(t);
  }

  /** Whether a and b are spelled the same. */
  bool equal(TNode a, TNode b)
  {
    TNode x = top(a);
    TNode y = top(b);
    if (x == y)
    {
      return true;
    }
    size_t n = numChildren(x);
    if (n == 0 || n != numChildren(y) || kind(x) != kind(y)
        || hasOperator(x) != hasOperator(y)
        || (hasOperator(x) && x.getOperator() != y.getOperator()))
    {
      return false;
    }
    if (commutativeChildren(kind(x)))
    {
      std::vector<bool> used(n, false);
      for (size_t i = 0; i < n; i++)
      {
        size_t j = 0;
        while (j < n && (used[j] || !equal(child(x, i), child(y, j))))
        {
          j++;
        }
        if (j == n)
        {
          return false;
        }
        used[j] = true;
      }
      return true;
    }
    for (size_t i = 0; i < n; i++)
    {
      if (!equal(child(x, i), child(y, i)))
      {
        return false;
      }
    }
    return true;
  }

  /** A hash of the spelling of n, consistent with equal. */
  size_t hash(TNode n)
  {
    bool ground = !expr::hasBoundVar(n);
    if (ground)
    {
      auto it = d_hash.find(n);
      if (it != d_hash.end())
      {
        return it->second;
      }
    }
    TNode x = top(n);
    size_t h;
    size_t nc = numChildren(x);
    if (nc == 0)
    {
      h = std::hash<TNode>()(x);
    }
    else
    {
      h = static_cast<size_t>(kind(x));
      if (hasOperator(x))
      {
        h = combine(h, std::hash<TNode>()(x.getOperator()));
      }
      if (commutativeChildren(kind(x)))
      {
        size_t acc = 0;
        for (size_t i = 0; i < nc; i++)
        {
          acc = combineUnordered(acc, hash(child(x, i)));
        }
        h = combine(h, acc);
      }
      else
      {
        for (size_t i = 0; i < nc; i++)
        {
          h = combine(h, hash(child(x, i)));
        }
      }
    }
    if (ground)
    {
      d_hash[n] = h;
    }
    return h;
  }

  /** Print n as the input spells it. */
  void print(std::ostream& os, TNode n)
  {
    TNode x = top(n);
    if (!respelled(x))
    {
      os << x;
      return;
    }
    os << "(";
    if (hasOperator(x))
    {
      os << x.getOperator();
    }
    else
    {
      os << printer::smt2::Smt2Printer::smtKindString(kind(x));
    }
    for (size_t i = 0, nc = numChildren(x); i < nc; i++)
    {
      os << " ";
      print(os, child(x, i));
    }
    os << ")";
  }

 private:
  /** Whether n is spelled differently from how the printer would print it. */
  bool respelled(TNode n)
  {
    bool ground = !expr::hasBoundVar(n);
    if (!ground)
    {
      return true;
    }
    auto it = d_respelled.find(n);
    if (it != d_respelled.end())
    {
      return it->second;
    }
    bool ret = top(n) != n;
    if (!ret && !isLeaf(n))
    {
      ret = isGuardedDivision(n) || partialKind(n.getKind()) != n.getKind();
      for (size_t i = 0, nc = n.getNumChildren(); !ret && i < nc; i++)
      {
        ret = respelled(n[i]);
      }
    }
    d_respelled[n] = ret;
    return ret;
  }

  std::unordered_map<TNode, TNode> d_bind;
  std::unordered_map<TNode, size_t> d_hash;
  std::unordered_map<TNode, bool> d_respelled;
};

/** Whether the arguments of f are distinct bound variables. */
bool overBoundVars(TNode f)
{
  std::unordered_set<TNode> seen;
  for (TNode x : f)
  {
    if (x.getKind() != Kind::BOUND_VARIABLE || !seen.insert(x).second)
    {
      return false;
    }
  }
  return true;
}

/**
 * The uninterpreted functions the input defines as an operation that can
 * host an atom, mapped to it: f wraps op when a quantifier body holds
 * (= (f x1 ... xn) (op x1 ... xn)), or the same with a wrapper of op for op,
 * over the same distinct bound variables in the same order. Verus's prelude
 * defines Mul, EucDiv and EucMod so; its Add and Sub wrap no such operation.
 * A ground equation such as (= (g a b) (* a b)) defines g at one point only,
 * so it makes no wrapper.
 */
std::unordered_map<Node, Kind> findWrappers(const std::vector<Node>& input)
{
  std::unordered_map<Node, Kind> wrappers;
  std::vector<std::pair<Node, Node>> links;
  std::unordered_set<TNode> visited;
  std::vector<TNode> visit(input.begin(), input.end());
  while (!visit.empty())
  {
    TNode cur = visit.back();
    visit.pop_back();
    if (!visited.insert(cur).second)
    {
      continue;
    }
    if (cur.isClosure())
    {
      visit.push_back(cur[1]);
      continue;
    }
    visit.insert(visit.end(), cur.begin(), cur.end());
    if (cur.getKind() != Kind::EQUAL)
    {
      continue;
    }
    for (size_t i = 0; i < 2; i++)
    {
      TNode f = cur[i];
      TNode g = cur[1 - i];
      if (f.getKind() != Kind::APPLY_UF || !overBoundVars(f) || g.isClosure()
          || g.getNumChildren() != f.getNumChildren()
          || !std::equal(f.begin(), f.end(), g.begin()))
      {
        continue;
      }
      if (g.getKind() == Kind::APPLY_UF)
      {
        links.emplace_back(f.getOperator(), g.getOperator());
        continue;
      }
      Kind op = operationOf(partialKind(g.getKind()));
      if (op != Kind::UNDEFINED_KIND)
      {
        wrappers.emplace(f.getOperator(), op);
      }
    }
  }
  bool changed = true;
  while (changed)
  {
    changed = false;
    for (const auto& [f, g] : links)
    {
      auto it = wrappers.find(g);
      if (it != wrappers.end() && wrappers.emplace(f, it->second).second)
      {
        changed = true;
      }
    }
  }
  return wrappers;
}

/** What a host must apply to host an atom, and to which operands. */
struct AtomKey
{
  Kind d_op = Kind::UNDEFINED_KIND;
  std::vector<TNode> d_args;
};

class HostMatcher
{
 public:
  HostMatcher(SourceView& sv, std::unordered_map<Node, Kind> wrappers)
      : d_sv(sv), d_wrappers(std::move(wrappers))
  {
  }

  /** The operation the term t (returned by top) applies, see operationOf. */
  Kind operation(TNode t) const
  {
    if (d_sv.isLeaf(t))
    {
      return Kind::UNDEFINED_KIND;
    }
    Kind k = d_sv.kind(t);
    if (k == Kind::APPLY_UF)
    {
      auto it = d_wrappers.find(t.getOperator());
      return it == d_wrappers.end() ? Kind::UNDEFINED_KIND : it->second;
    }
    return operationOf(k);
  }

  /**
   * The key of the term t applying operation op: its operands, with nested
   * products (through any wrapper) flattened. The constant factors of a
   * builtin product are dropped, as the rewriter drops them from the atom:
   * (* 2 a b) hosts (* a b). A wrapper's constant arguments are kept, since
   * arithmetic sees the wrapper as an opaque term: (Mul (Mul 2 v) v) hosts
   * (* v (Mul 2 v)), not (* v v).
   */
  AtomKey key(TNode t, Kind op) const
  {
    AtomKey k;
    k.d_op = op;
    addOperands(d_sv.top(t), op, k.d_args);
    return k;
  }

  /**
   * The factors of t: the operands of key(t, NONLINEAR_MULT) when t is a
   * product, and t itself when it is anything else. A product's factors are
   * what arithmetic multiplies, so a wrapped one is flattened and a constant
   * one dropped, exactly as in an atom.
   */
  std::vector<TNode> factors(TNode t) const
  {
    TNode x = d_sv.top(t);
    if (operation(x) == Kind::NONLINEAR_MULT)
    {
      return key(x, Kind::NONLINEAR_MULT).d_args;
    }
    return {x};
  }

  /** Where key k is looked up. */
  std::pair<Kind, std::vector<size_t>> index(const AtomKey& k) const
  {
    std::vector<size_t> hs;
    for (TNode a : k.d_args)
    {
      hs.push_back(d_sv.hash(a));
    }
    if (commutes(k.d_op))
    {
      std::sort(hs.begin(), hs.end());
    }
    return {k.d_op, hs};
  }

  /** Whether a and b have one operation and the same operands. */
  bool same(const AtomKey& a, const AtomKey& b) const
  {
    size_t n = a.d_args.size();
    if (a.d_op != b.d_op || n != b.d_args.size())
    {
      return false;
    }
    if (!commutes(a.d_op))
    {
      for (size_t i = 0; i < n; i++)
      {
        if (!d_sv.equal(a.d_args[i], b.d_args[i]))
        {
          return false;
        }
      }
      return true;
    }
    std::vector<bool> used(n, false);
    for (TNode x : a.d_args)
    {
      size_t j = 0;
      while (j < n && (used[j] || !d_sv.equal(x, b.d_args[j])))
      {
        j++;
      }
      if (j == n)
      {
        return false;
      }
      used[j] = true;
    }
    return true;
  }

 private:
  void addOperands(TNode t, Kind op, std::vector<TNode>& out) const
  {
    for (size_t i = 0, n = d_sv.numChildren(t); i < n; i++)
    {
      TNode c = d_sv.top(d_sv.child(t, i));
      if (op == Kind::NONLINEAR_MULT && c.isConst()
          && d_sv.kind(t) != Kind::APPLY_UF)
      {
        continue;
      }
      if (op == Kind::NONLINEAR_MULT && operation(c) == op)
      {
        addOperands(c, op, out);
        continue;
      }
      out.push_back(c);
    }
  }

  SourceView& d_sv;
  std::unordered_map<Node, Kind> d_wrappers;
};

/**
 * The division that extended term x stands for, or null: operator
 * elimination multiplies the term purifying a division or modulus by its
 * divisor, (* d q) for q purifying (div n d), and the rewriter then folds
 * that product into one, so the atom of (div n (* b c)) is (* b c q) and the
 * atom of (div n (* 2 b)) is (* b q). It is that product when the factors
 * other than q are exactly the factors of d. Any other product with such a
 * factor, (* c (div a b)) with c not b, is a product the input wrote, which
 * keeps the division as an opaque operand.
 */
TNode eliminatedDivision(const HostMatcher& hm, SourceView& sv, const Node& x)
{
  if (x.getKind() != Kind::NONLINEAR_MULT)
  {
    return TNode();
  }
  for (size_t i = 0, n = x.getNumChildren(); i < n; i++)
  {
    TNode t = sv.top(x[i]);
    if (sv.isLeaf(t) || !isDivisionKind(sv.kind(t)))
    {
      continue;
    }
    AtomKey rest;
    rest.d_op = Kind::NONLINEAR_MULT;
    for (size_t j = 0; j < n; j++)
    {
      if (j != i)
      {
        std::vector<TNode> fs = hm.factors(x[j]);
        rest.d_args.insert(rest.d_args.end(), fs.begin(), fs.end());
      }
    }
    AtomKey divisor;
    divisor.d_op = Kind::NONLINEAR_MULT;
    divisor.d_args = hm.factors(sv.child(t, 1));
    if (hm.same(rest, divisor))
    {
      return t;
    }
  }
  return TNode();
}

/** The frontier kind of extended term x. */
const char* frontierKind(const HostMatcher& hm, SourceView& sv, const Node& x)
{
  switch (x.getKind())
  {
    case Kind::IAND:
    case Kind::PIAND: return "iand";
    case Kind::POW2: return "pow2";
    case Kind::EXPONENTIAL:
    case Kind::SINE:
    case Kind::PI: return "transcendental";
    default: break;
  }
  if (!eliminatedDivision(hm, sv, x).isNull())
  {
    return "division";
  }
  for (const Node& f : x)
  {
    if (f != x[0])
    {
      return "product";
    }
  }
  return "power";
}

/**
 * The :qid of quantified formula q, or empty if it has none. Read from the
 * attributes computed when q was registered: computing them again would
 * build nodes.
 */
std::string quantName(theory::QuantifiersEngine* qe, const Node& q)
{
  Node name = qe->getNameForQuant(q);
  if (name.isNull() || name == q || !name.hasName())
  {
    return "";
  }
  return name.getName();
}

struct Host
{
  /** The hosting term, as the input spells it. */
  std::string d_term;
  /** Input hosts: the term, and the tags of the input assertions holding it. */
  TNode d_node;
  std::vector<std::string> d_tags;
  /**
   * Instantiation hosts: the quantifier, and how many of its instantiations
   * produced a host (the last of them, 1-based, in d_lastInstance).
   */
  std::string d_qid;
  size_t d_count = 0;
  size_t d_lastInstance = 0;
};

struct AtomHosts
{
  std::vector<Host> d_input;
  std::vector<Host> d_instance;
};

/** The atom itself, as the argument index of a host target. */
constexpr size_t kAtomTerm = static_cast<size_t>(-1);

/**
 * A term the reply reports hosts for: an atom, or one of its arguments. An
 * argument carries the provenance when the atom around it is one the
 * rewriter built rather than one the input wrote, as in
 * (* b (div a (+ 1 b))), where distributing operator elimination's
 * (* (+ 1 b) q) left a product no input term applies.
 */
struct HostTarget
{
  /** Its atom, as an index into the reported order. */
  size_t d_pos = 0;
  /** Which term of that atom: kAtomTerm, or an index into its arguments. */
  size_t d_arg = kAtomTerm;
};

/**
 * The keys a term keyed by k is hosted by. An integer division also reads a
 * modulus the input wrote: operator elimination rewrites (mod n d) as
 * n - d * (div n d), so the division arithmetic sees can be where a modulus
 * entered the problem, as Verus's EucMod does.
 */
std::vector<AtomKey> hostKeys(AtomKey k)
{
  std::vector<AtomKey> ks;
  if (k.d_op == Kind::INTS_DIVISION)
  {
    AtomKey m = k;
    m.d_op = Kind::INTS_MODULUS;
    ks.push_back(std::move(m));
  }
  ks.push_back(std::move(k));
  return ks;
}

/** The hosts of one term, as the reply lists them. */
void printHosts(std::ostream& os, const AtomHosts* hs)
{
  os << " :hosts (";
  bool first = true;
  if (hs != nullptr)
  {
    for (const Host& h : hs->d_input)
    {
      os << (first ? "" : " ") << "(:in input :term " << h.d_term << " :tags (";
      for (size_t i = 0, n = h.d_tags.size(); i < n; i++)
      {
        os << (i > 0 ? " " : "") << quoteSymbol(h.d_tags[i]);
      }
      os << "))";
      first = false;
    }
    for (const Host& h : hs->d_instance)
    {
      os << (first ? "" : " ") << "(:in instance :term " << h.d_term << " :qid "
         << (h.d_qid.empty() ? "none" : quoteSymbol(h.d_qid)) << " :count "
         << h.d_count << ")";
      first = false;
    }
  }
  os << ")";
}

/** A model value: a rational as -5 or 1/2, or none when it was not one. */
void printValue(std::ostream& os, bool has, const Rational& v)
{
  if (has)
  {
    os << v;
  }
  else
  {
    os << "none";
  }
}

void printBound(std::ostream& os, const char* key, const NlFrontierBound& b)
{
  if (!b.d_set)
  {
    return;
  }
  // a Rational prints as -5 or 1/2, not in SMT-LIB syntax
  os << " " << key << " (:value " << b.d_value << " :strict "
     << (b.d_strict ? "true" : "false") << " :fixed "
     << (b.d_fixed ? "true" : "false") << ")";
}

}  // namespace

std::string getNlFrontierInfo(TheoryEngine* te,
                              theory::QuantifiersEngine* qe,
                              const Assertions* as,
                              const std::string& result,
                              const std::string& reason)
{
  const NonlinearExtension* nl = nullptr;
  if (te != nullptr)
  {
    theory::Theory* t = te->theoryOf(theory::THEORY_ARITH);
    if (t != nullptr)
    {
      nl = static_cast<theory::arith::TheoryArith*>(t)->getNonlinearExtension();
    }
  }
  std::stringstream ss;
  // whole terms, without let-bindings for shared subterms
  options::ioutils::applyDagThresh(ss, 0);
  ss << "(:result " << result << " :reason " << reason << " :enabled "
     << (nl != nullptr ? "true" : "false");
  // With no result to report there is no record either: after a push the
  // extension still holds the last check's atoms, but the reply would have
  // no assertions or instantiations to find their hosts in.
  if (nl == nullptr || result == "none")
  {
    ss << " :checks 0 :rounds 0 :punts 0 :last none :atoms () :omitted 0 "
          ":truncated false)";
    return ss.str();
  }
  const NlFrontier& f = nl->getFrontier();
  SourceView sv;

  // The atoms of the most recent round first, then by how many rounds they
  // were wrong in, then in the order they were first found. Atoms the input
  // spells the same (a division reaching the extension both guarded and
  // unguarded, say) are reported once.
  std::vector<size_t> order(f.d_atoms.size());
  for (size_t i = 0; i < order.size(); i++)
  {
    order[i] = i;
  }
  auto current = [&f](const NlFrontierAtom& a) {
    return f.d_lastRound > 0 && a.d_lastRound == f.d_lastRound;
  };
  std::stable_sort(order.begin(), order.end(), [&](size_t i, size_t j) {
    const NlFrontierAtom& a = f.d_atoms[i];
    const NlFrontierAtom& b = f.d_atoms[j];
    if (current(a) != current(b))
    {
      return current(a);
    }
    return a.d_rounds > b.d_rounds;
  });
  std::vector<size_t> distinct;
  for (size_t i : order)
  {
    const Node& x = f.d_atoms[i].d_atom.d_term;
    if (std::none_of(distinct.begin(), distinct.end(), [&](size_t j) {
          const Node& y = f.d_atoms[j].d_atom.d_term;
          return sv.hash(x) == sv.hash(y) && sv.equal(x, y);
        }))
    {
      distinct.push_back(i);
    }
  }
  order = distinct;
  size_t omitted = 0;
  if (order.size() > kMaxAtoms)
  {
    omitted = order.size() - kMaxAtoms;
    order.resize(kMaxAtoms);
  }

  std::vector<Node> input;
  if (as != nullptr)
  {
    std::unordered_set<Node> seen;
    for (const Node& a : as->getAssertionList())
    {
      if (seen.insert(a).second)
      {
        input.push_back(a);
      }
    }
  }
  HostMatcher hm(sv, findWrappers(input));

  // Every term hosts are looked for, with its key: the operation computing it
  // and its operands. An atom, keyed for operator elimination's (* d q) by
  // the dividend and divisor of the division q purifies, and each argument
  // that applies an operation of its own, since the rewriter can leave an
  // atom no input term applies around an argument the input did write.
  std::vector<HostTarget> targets;
  std::vector<std::vector<AtomKey>> keys;
  std::map<std::pair<Kind, std::vector<size_t>>, std::vector<size_t>> byKey;
  auto addTarget = [&](size_t pos, size_t arg, AtomKey key) {
    if (key.d_op == Kind::UNDEFINED_KIND || key.d_args.empty())
    {
      return;
    }
    std::vector<AtomKey> ks = hostKeys(std::move(key));
    for (const AtomKey& k : ks)
    {
      std::vector<size_t>& bucket = byKey[hm.index(k)];
      if (bucket.empty() || bucket.back() != targets.size())
      {
        bucket.push_back(targets.size());
      }
    }
    targets.push_back(HostTarget{pos, arg});
    keys.push_back(std::move(ks));
  };
  for (size_t pos = 0; pos < order.size(); pos++)
  {
    const NlFrontierAtom& a = f.d_atoms[order[pos]];
    const Node& x = a.d_atom.d_term;
    TNode d = eliminatedDivision(hm, sv, x);
    addTarget(pos,
              kAtomTerm,
              d.isNull() ? hm.key(x, operationOf(x.getKind()))
                         : hm.key(d, sv.kind(d)));
    for (size_t i = 0, n = a.d_args.size(); i < n; i++)
    {
      TNode t = sv.top(a.d_args[i].d_term);
      Kind op = hm.operation(t);
      if (op != Kind::UNDEFINED_KIND)
      {
        addTarget(pos, i, hm.key(t, op));
      }
    }
  }
  std::vector<AtomHosts> hosts(targets.size());
  // The targets term t (applying operation op) hosts.
  auto hosted = [&](TNode t, Kind op) {
    std::vector<size_t> ret;
    AtomKey k = hm.key(t, op);
    auto it = byKey.find(hm.index(k));
    if (it != byKey.end())
    {
      for (size_t ti : it->second)
      {
        if (std::any_of(keys[ti].begin(),
                        keys[ti].end(),
                        [&](const AtomKey& tk) { return hm.same(k, tk); }))
        {
          ret.push_back(ti);
        }
      }
    }
    return ret;
  };

  // Hosts in the input: ground terms of the input assertions applying an
  // atom's operation, or a wrapper of it, to exactly its operands, with the
  // tags of every assertion holding them. Quantifier bodies are skipped
  // here; their terms are found instantiated below.
  if (!byKey.empty())
  {
    for (const Node& a : input)
    {
      std::vector<std::string> tags;
      as->getAssertionTags(a, tags);
      std::unordered_set<TNode> visited;
      std::vector<TNode> visit{a};
      while (!visit.empty())
      {
        TNode cur = visit.back();
        visit.pop_back();
        if (!visited.insert(cur).second || cur.isClosure())
        {
          continue;
        }
        visit.insert(visit.end(), cur.begin(), cur.end());
        Kind op = hm.operation(sv.top(cur));
        if (op == Kind::UNDEFINED_KIND)
        {
          continue;
        }
        for (size_t ti : hosted(cur, op))
        {
          std::vector<Host>& hs = hosts[ti].d_input;
          auto h = std::find_if(hs.begin(), hs.end(), [&](const Host& e) {
            return sv.equal(e.d_node, cur);
          });
          if (h == hs.end())
          {
            if (hs.size() >= kMaxHosts)
            {
              continue;
            }
            std::stringstream ts;
            options::ioutils::applyDagThresh(ts, 0);
            sv.print(ts, cur);
            hs.push_back(Host{ts.str(), cur, {}, "", 0, 0});
            h = hs.end() - 1;
          }
          for (const std::string& tag : tags)
          {
            if (std::find(h->d_tags.begin(), h->d_tags.end(), tag)
                == h->d_tags.end())
            {
              h->d_tags.push_back(tag);
            }
          }
        }
      }
    }
  }

  // Hosts in instantiations: each term of a quantified formula's body that
  // mentions its variables and applies an atom's operation (or a wrapper of
  // it) to exactly its operands under some instantiation. Wrapper
  // applications are tried first, so a host reads (Mul a b) rather than the
  // (* a b) its definition unfolds to.
  bool truncated = false;
  if (qe != nullptr && !byKey.empty())
  {
    std::vector<Node> qs;
    qe->getInstantiatedQuantifiedFormulas(qs);
    size_t work = 0;
    for (const Node& q : qs)
    {
      if (truncated)
      {
        break;
      }
      std::vector<std::pair<TNode, Kind>> candidates;
      std::unordered_set<TNode> visited;
      std::vector<TNode> visit{q[1]};
      while (!visit.empty())
      {
        TNode cur = visit.back();
        visit.pop_back();
        if (!visited.insert(cur).second || cur.isClosure())
        {
          continue;
        }
        // left to right
        for (size_t i = cur.getNumChildren(); i > 0; i--)
        {
          visit.push_back(cur[i - 1]);
        }
        Kind op = hm.operation(cur);
        if (op != Kind::UNDEFINED_KIND && expr::hasBoundVar(cur))
        {
          candidates.emplace_back(cur, op);
        }
      }
      if (candidates.empty())
      {
        continue;
      }
      std::stable_partition(
          candidates.begin(), candidates.end(), [](const auto& c) {
            return c.first.getKind() == Kind::APPLY_UF;
          });
      std::vector<Node> vars(q[0].begin(), q[0].end());
      std::vector<std::vector<Node>> tvecs;
      qe->getInstantiationTermVectors(q, tvecs);
      std::string qid;
      for (size_t vi = 0, nv = tvecs.size(); vi < nv; vi++)
      {
        const std::vector<Node>& tv = tvecs[vi];
        if (tv.size() != vars.size())
        {
          continue;
        }
        work += candidates.size();
        if (work > kMaxInstanceWork)
        {
          truncated = true;
          break;
        }
        sv.bind(vars, tv);
        for (const auto& [cand, op] : candidates)
        {
          std::vector<size_t> ps = hosted(cand, op);
          if (ps.empty())
          {
            continue;
          }
          if (qid.empty())
          {
            qid = quantName(qe, q);
          }
          for (size_t ti : ps)
          {
            std::vector<Host>& hs = hosts[ti].d_instance;
            auto h = std::find_if(hs.begin(), hs.end(), [&](const Host& e) {
              return e.d_qid == qid;
            });
            if (h == hs.end())
            {
              if (hs.size() >= kMaxHosts)
              {
                continue;
              }
              std::stringstream ts;
              options::ioutils::applyDagThresh(ts, 0);
              sv.print(ts, cand);
              hs.push_back(Host{ts.str(), TNode(), {}, qid, 0, 0});
              h = hs.end() - 1;
            }
            if (h->d_lastInstance != vi + 1)
            {
              h->d_lastInstance = vi + 1;
              h->d_count++;
            }
          }
        }
      }
      sv.unbind();
    }
  }

  // the host list to print for each atom and for each of its arguments
  std::vector<const AtomHosts*> atomHosts(order.size(), nullptr);
  std::vector<std::vector<const AtomHosts*>> argHosts(order.size());
  for (size_t pos = 0; pos < order.size(); pos++)
  {
    argHosts[pos].assign(f.d_atoms[order[pos]].d_args.size(), nullptr);
  }
  for (size_t ti = 0; ti < targets.size(); ti++)
  {
    const HostTarget& t = targets[ti];
    if (t.d_arg == kAtomTerm)
    {
      atomHosts[t.d_pos] = &hosts[ti];
    }
    else
    {
      argHosts[t.d_pos][t.d_arg] = &hosts[ti];
    }
  }

  ss << " :checks " << f.d_checks << " :rounds " << f.d_rounds << " :punts "
     << f.d_punts << " :last " << f.d_last << " :atoms (";
  for (size_t pos = 0; pos < order.size(); pos++)
  {
    const NlFrontierAtom& a = f.d_atoms[order[pos]];
    ss << (pos > 0 ? " " : "") << "(:atom ";
    sv.print(ss, a.d_atom.d_term);
    ss << " :kind " << frontierKind(hm, sv, a.d_atom.d_term) << " :current "
       << (current(a) ? "true" : "false") << " :rounds " << a.d_rounds
       << " :value ";
    printValue(ss, a.d_atom.d_hasValue, a.d_atom.d_value);
    ss << " :from-args ";
    printValue(ss, a.d_hasFromArgs, a.d_fromArgs);
    printBound(ss, ":lower", a.d_atom.d_lower);
    printBound(ss, ":upper", a.d_atom.d_upper);
    ss << " :args (";
    for (size_t i = 0, n = a.d_args.size(); i < n; i++)
    {
      const NlFrontierTerm& t = a.d_args[i];
      ss << (i > 0 ? " " : "") << "(:term ";
      sv.print(ss, t.d_term);
      ss << " :value ";
      printValue(ss, t.d_hasValue, t.d_value);
      printBound(ss, ":lower", t.d_lower);
      printBound(ss, ":upper", t.d_upper);
      printHosts(ss, argHosts[pos][i]);
      ss << ")";
    }
    ss << ")";
    printHosts(ss, atomHosts[pos]);
    ss << ")";
  }
  ss << ") :omitted " << omitted << " :truncated "
     << (truncated ? "true" : "false") << ")";
  return ss.str();
}

}  // namespace smt
}  // namespace cvc5::internal
