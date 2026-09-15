/******************************************************************************
 * This file is part of the cvc5 project.
 *
 * Copyright (c) 2009-2026 by the authors listed in the file AUTHORS
 * in the top-level source directory and their institutional affiliations.
 * All rights reserved.  See the file COPYING in the top-level source
 * directory for licensing information.
 * ****************************************************************************
 *
 * Speculative hypotheses about quantifier instantiation, scoped to a user
 * context.
 */

#include "theory/quantifiers/speculation.h"

#include <algorithm>
#include <cctype>
#include <functional>
#include <sstream>

#include "base/modal_exception.h"
#include "expr/node_algorithm.h"
#include "expr/skolem_manager.h"
#include "options/io_utils.h"
#include "options/quantifiers_options.h"
#include "printer/smt2/smt2_printer.h"
#include "theory/quantifiers/ematching/pattern_term_selector.h"
#include "theory/quantifiers/ematching/trigger.h"
#include "theory/quantifiers/instantiate.h"
#include "theory/quantifiers/quantifiers_attributes.h"
#include "theory/quantifiers/quantifiers_registry.h"
#include "theory/quantifiers/quantifiers_state.h"
#include "theory/quantifiers/term_util.h"
#include "theory/trust_substitutions.h"
#include "util/rational.h"
#include "util/smt2_quote_string.h"

using namespace cvc5::internal::kind;

namespace cvc5::internal {
namespace theory {
namespace quantifiers {

namespace {

/** Instance vectors kept per hypothesis */
constexpr size_t kMaxInstances = 20;
/** The most characters a printed term may take */
constexpr size_t kMaxTermChars = 1000;

/** n flat, or the symbol ... when that is longer than kMaxTermChars */
std::string printTerm(const Node& n)
{
  std::stringstream ss;
  options::ioutils::applyDagThresh(ss, 0);
  ss << n;
  return ss.str().size() <= kMaxTermChars ? ss.str() : "...";
}

/** s without the bars that quote an SMT-LIB symbol */
std::string stripBars(const std::string& s)
{
  if (s.size() >= 2 && s.front() == '|' && s.back() == '|')
  {
    return s.substr(1, s.size() - 2);
  }
  return s;
}

/** The name of a variable, as its binder spells it */
std::string nameOf(const Node& v)
{
  return v.hasName() ? v.getName() : stripBars(printTerm(v));
}

void printTerms(std::ostream& out, const std::vector<Node>& ts)
{
  out << "(";
  for (size_t i = 0, n = ts.size(); i < n; i++)
  {
    out << (i == 0 ? "" : " ") << printTerm(ts[i]);
  }
  out << ")";
}

void printVectors(std::ostream& out, const std::vector<std::vector<Node>>& vs)
{
  out << "(";
  for (size_t i = 0, n = vs.size(); i < n; i++)
  {
    out << (i == 0 ? "" : " ");
    printTerms(out, vs[i]);
  }
  out << ")";
}

}  // namespace

Speculation::PopNotify::PopNotify(context::Context* c, Speculation& s)
    : ContextNotifyObj(c), d_spec(s)
{
}

void Speculation::PopNotify::contextNotifyPop()
{
  // Hold no term of a popped scope: a term kept alive would change which
  // node ids later terms get, and with them the search.
  d_spec.d_qids.clear();
  d_spec.d_depth.clear();
  d_spec.d_current = nullptr;
}

Speculation::Speculation(Env& env,
                         QuantifiersState& qs,
                         QuantifiersInferenceManager& qim,
                         QuantifiersRegistry& qr,
                         TermRegistry& tr)
    : EnvObj(env),
      d_envRef(env),
      d_qstate(qs),
      d_qim(qim),
      d_qreg(qr),
      d_treg(tr),
      d_hyps(userContext()),
      d_threshold(userContext(), kDefaultLoopThreshold),
      d_done(userContext()),
      d_triggers(userContext()),
      d_popNotify(userContext(), *this)
{
}

Speculation::~Speculation() {}

bool Speculation::Fingerprint::isHole() const
{
  if (d_list || d_atom.empty())
  {
    return false;
  }
  if (d_atom == "_")
  {
    return true;
  }
  if (d_atom.size() < 2 || (d_atom[0] != '_' && d_atom[0] != '#'))
  {
    return false;
  }
  return std::all_of(d_atom.begin() + 1, d_atom.end(), [](char c) {
    return c >= '0' && c <= '9';
  });
}

std::string Speculation::Fingerprint::text() const
{
  if (!d_list)
  {
    return d_atom;
  }
  std::string out = "(";
  for (size_t i = 0, n = d_kids.size(); i < n; i++)
  {
    out += (i == 0 ? "" : " ") + d_kids[i].text();
  }
  return out + ")";
}

Speculation::Fingerprint Speculation::parseFingerprint(const std::string& text)
{
  // Tokens: parentheses, |quoted| symbols (unquoted here), and atoms.
  std::vector<std::string> toks;
  for (size_t i = 0, n = text.size(); i < n;)
  {
    char c = text[i];
    if (std::isspace(static_cast<unsigned char>(c)))
    {
      i++;
    }
    else if (c == '(' || c == ')')
    {
      toks.emplace_back(1, c);
      i++;
    }
    else if (c == '|')
    {
      size_t end = text.find('|', i + 1);
      if (end == std::string::npos)
      {
        throw RecoverableModalException("speculate: unclosed | in fingerprint");
      }
      toks.push_back(text.substr(i + 1, end - i - 1));
      i = end + 1;
    }
    else
    {
      size_t j = i;
      while (j < n && !std::isspace(static_cast<unsigned char>(text[j]))
             && text[j] != '(' && text[j] != ')')
      {
        j++;
      }
      toks.push_back(text.substr(i, j - i));
      i = j;
    }
  }
  size_t pos = 0;
  std::function<Fingerprint()> term = [&]() -> Fingerprint {
    if (pos >= toks.size())
    {
      throw RecoverableModalException("speculate: incomplete fingerprint");
    }
    Fingerprint f;
    if (toks[pos] == ")")
    {
      throw RecoverableModalException("speculate: unbalanced ) in fingerprint");
    }
    if (toks[pos] != "(")
    {
      f.d_atom = toks[pos++];
      return f;
    }
    pos++;
    f.d_list = true;
    while (pos < toks.size() && toks[pos] != ")")
    {
      f.d_kids.push_back(term());
    }
    if (pos >= toks.size())
    {
      throw RecoverableModalException("speculate: unclosed ( in fingerprint");
    }
    pos++;
    if (f.d_kids.size() < 2 || f.d_kids[0].d_list || f.d_kids[0].isHole())
    {
      throw RecoverableModalException(
          "speculate: a fingerprint application needs a symbol at its head "
          "and at least one argument");
    }
    return f;
  };
  Fingerprint f = term();
  if (pos != toks.size())
  {
    throw RecoverableModalException(
        "speculate: a fingerprint is a single term");
  }
  return f;
}

bool Speculation::matches(const Node& n,
                          const Fingerprint& p,
                          std::map<std::string, Node>& holes) const
{
  if (p.isHole())
  {
    if (p.d_atom == "_")
    {
      return true;
    }
    auto [it, fresh] = holes.emplace(p.d_atom, n);
    return fresh || it->second == n;
  }
  // A leaf constant prints as a token or as an application such as (- 1) or
  // (/ 1 2); either way it has no children, so compare the texts.
  if (n.isConst() && n.getNumChildren() == 0)
  {
    return stripBars(printTerm(n)) == p.text();
  }
  if (!p.d_list)
  {
    return n.getNumChildren() == 0 && stripBars(printTerm(n)) == p.d_atom;
  }
  if (n.getNumChildren() + 1 != p.d_kids.size())
  {
    return false;
  }
  std::string head = n.getMetaKind() == kind::metakind::PARAMETERIZED
                         ? stripBars(printTerm(n.getOperator()))
                         : printer::smt2::Smt2Printer::smtKindStringOf(n);
  if (head != p.d_kids[0].d_atom)
  {
    return false;
  }
  for (size_t i = 0, k = n.getNumChildren(); i < k; i++)
  {
    if (!matches(n[i], p.d_kids[i + 1], holes))
    {
      return false;
    }
  }
  return true;
}

void Speculation::add(const SpeculationRequest& r)
{
  auto h = std::make_shared<Hypothesis>();
  h->d_req = r;
  if (r.d_kind != SpeculationRequest::Kind::OBSERVE && r.d_qid.empty())
  {
    throw RecoverableModalException("speculate: a hypothesis needs a :qid");
  }
  switch (r.d_kind)
  {
    case SpeculationRequest::Kind::INSTANTIATE:
      if (r.d_names.empty() || r.d_names.size() != r.d_terms.size())
      {
        throw RecoverableModalException(
            "speculate: :instantiate needs a term for each variable named");
      }
      break;
    case SpeculationRequest::Kind::TRIGGER:
      if (r.d_pattern.empty())
      {
        throw RecoverableModalException(
            "speculate: :trigger needs at least one pattern term");
      }
      break;
    case SpeculationRequest::Kind::BLOCK:
      h->d_fp = parseFingerprint(r.d_fingerprint);
      break;
    case SpeculationRequest::Kind::OBSERVE: break;
  }
  if (r.d_loopThreshold != 0)
  {
    d_threshold = r.d_loopThreshold;
  }
  d_hyps.push_back(h);
}

bool Speculation::isActive() const { return d_hyps.size() > 0; }

void Speculation::presolve()
{
  d_depth.clear();
  d_blockedThisCheck = false;
}

const std::string& Speculation::qidOf(const Node& q) const
{
  auto it = d_qids.find(q);
  if (it != d_qids.end())
  {
    return it->second;
  }
  QAttributes qa;
  QuantAttributes::computeQuantAttributes(q, qa);
  std::string name = qa.d_name.isNull() || !qa.d_name.hasName()
                         ? std::string()
                         : qa.d_name.getName();
  return d_qids.emplace(q, name).first->second;
}

void Speculation::fail(Hypothesis& h,
                       const std::string& status,
                       std::string why)
{
  if (h.d_status != "applied")
  {
    h.d_status = status;
    h.d_reason = std::move(why);
  }
}

void Speculation::keep(Hypothesis& h, const std::vector<Node>& terms)
{
  if (h.d_instances.size() >= kMaxInstances)
  {
    return;
  }
  std::vector<Node> orig;
  for (const Node& t : terms)
  {
    orig.push_back(SkolemManager::getOriginalForm(t));
  }
  // a block refuses the same vector each round it is matched again
  if (std::find(h.d_instances.begin(), h.d_instances.end(), orig)
      == h.d_instances.end())
  {
    h.d_instances.push_back(orig);
  }
}

void Speculation::apply(Instantiate& inst, const std::vector<Node>& asserted)
{
  NodeManager* nm = nodeManager();
  for (size_t i = 0, n = d_hyps.size(); i < n; i++)
  {
    const std::shared_ptr<Hypothesis>& h = d_hyps[i];
    if (h->d_req.d_kind == SpeculationRequest::Kind::OBSERVE)
    {
      continue;
    }
    for (const Node& q : asserted)
    {
      if (d_qstate.isInConflict())
      {
        return;
      }
      if (qidOf(q) != h->d_req.d_qid)
      {
        continue;
      }
      Node key =
          NodeManager::mkNode(Kind::SEXPR, nm->mkConstInt(Rational(i)), q);
      if (d_done.find(key) != d_done.end())
      {
        continue;
      }
      d_done.insert(key);
      // a pop of a scope inside the hypothesis's own applies it again
      if (std::find(h->d_quants.begin(), h->d_quants.end(), q)
          == h->d_quants.end())
      {
        h->d_quants.push_back(q);
      }
      switch (h->d_req.d_kind)
      {
        case SpeculationRequest::Kind::INSTANTIATE:
          instantiate(inst, *h, q);
          break;
        case SpeculationRequest::Kind::TRIGGER: installTrigger(i, *h, q); break;
        case SpeculationRequest::Kind::BLOCK: h->d_status = "applied"; break;
        case SpeculationRequest::Kind::OBSERVE: break;
      }
    }
    h->d_considered = true;
  }
  // The speculative triggers match every round, before any strategy, so
  // their instances count as theirs rather than as a strategy's duplicates.
  struct Current
  {
    std::shared_ptr<Hypothesis>& d_slot;
    ~Current() { d_slot = nullptr; }
  } current{d_current};
  for (size_t i = 0, n = d_triggers.size(); i < n; i++)
  {
    if (d_qstate.isInConflict())
    {
      break;
    }
    const std::pair<size_t, std::shared_ptr<inst::Trigger>>& t = d_triggers[i];
    d_current = d_hyps[t.first];
    t.second->resetInstantiationRound();
    t.second->reset(Node::null());
    t.second->addInstantiations();
  }
}

void Speculation::instantiate(Instantiate& inst, Hypothesis& h, const Node& q)
{
  const SpeculationRequest& r = h.d_req;
  std::vector<Node> terms;
  for (const Node& v : q[0])
  {
    std::string name = nameOf(v);
    auto it = std::find(r.d_names.begin(), r.d_names.end(), name);
    if (it == r.d_names.end())
    {
      fail(h, "mismatch", "no term for the variable " + name);
      return;
    }
    Node t = rewrite(d_env.getTopLevelSubstitutions().apply(
        r.d_terms[it - r.d_names.begin()]));
    if (t.getType() != v.getType())
    {
      std::stringstream ss;
      ss << "the term for " << name << " has sort " << t.getType() << ", and "
         << name << " has sort " << v.getType();
      fail(h, "mismatch", ss.str());
      return;
    }
    terms.push_back(t);
  }
  for (const std::string& name : r.d_names)
  {
    if (std::none_of(q[0].begin(), q[0].end(), [&](const Node& v) {
          return nameOf(v) == name;
        }))
    {
      fail(h, "mismatch", "the formula binds no variable named " + name);
      return;
    }
  }
  Instantiate::Pressure before;
  auto pit = inst.getPressure().find(q);
  if (pit != inst.getPressure().end())
  {
    before = pit->second;
  }
  std::vector<Node> vec = terms;
  d_directing = true;
  bool added =
      inst.addInstantiation(q, vec, InferenceId::QUANTIFIERS_INST_LLM_DIRECTED);
  d_directing = false;
  if (added)
  {
    h.d_added++;
    h.d_status = "applied";
    h.d_reason.clear();
    keep(h, terms);
    Node body = SkolemManager::getOriginalForm(inst.getInstantiation(q, terms));
    if (h.d_bodies.size() < kMaxInstances
        && std::find(h.d_bodies.begin(), h.d_bodies.end(), body)
               == h.d_bodies.end())
    {
      h.d_bodies.push_back(body);
    }
    return;
  }
  h.d_rejected++;
  Instantiate::Pressure after;
  pit = inst.getPressure().find(q);
  if (pit != inst.getPressure().end())
  {
    after = pit->second;
  }
  if (after.d_dupEq > before.d_dupEq)
  {
    fail(h, "rejected", "this instantiation of the formula was made already");
  }
  else if (after.d_dupLemma > before.d_dupLemma)
  {
    fail(h, "rejected", "the instance is a lemma already sent");
  }
  else
  {
    fail(h, "rejected", "the instantiation level limit refused a term");
  }
}

void Speculation::installTrigger(size_t i, Hypothesis& h, const Node& q)
{
  const SpeculationRequest& r = h.d_req;
  std::vector<Node> from;
  std::vector<Node> to;
  for (const Node& pv : r.d_vars)
  {
    std::string name = nameOf(pv);
    auto it = std::find_if(q[0].begin(), q[0].end(), [&](const Node& v) {
      return nameOf(v) == name;
    });
    if (it == q[0].end())
    {
      fail(h, "mismatch", "the formula binds no variable named " + name);
      return;
    }
    if ((*it).getType() != pv.getType())
    {
      std::stringstream ss;
      ss << "the variable " << name << " has sort " << (*it).getType()
         << ", not " << pv.getType();
      fail(h, "mismatch", ss.str());
      return;
    }
    from.push_back(pv);
    to.push_back(*it);
  }
  std::vector<Node> shown;
  std::vector<Node> nodes;
  inst::PatternTermSelector pts(options(), q, options::TriggerSelMode::ALL);
  for (const Node& p : r.d_pattern)
  {
    Node ps = p.substitute(from.begin(), from.end(), to.begin(), to.end());
    shown.push_back(ps);
    Node pic = d_qreg.substituteBoundVariablesToInstConstants(ps, q);
    Node use = pts.getIsUsableTrigger(pic, q);
    if (use.isNull())
    {
      fail(h,
           "unusable",
           "cannot match with " + printTerm(ps)
               + ": a trigger term applies an uninterpreted function to "
                 "terms containing the formula's variables");
      return;
    }
    nodes.push_back(use);
  }
  // A pattern that leaves a variable unbound would instantiate it with an
  // arbitrary term of its sort.
  for (size_t j = 0, n = q[0].getNumChildren(); j < n; j++)
  {
    Node ic = d_qreg.getInstantiationConstant(q, j);
    if (std::none_of(nodes.begin(), nodes.end(), [&](const Node& u) {
          return expr::hasSubterm(u, ic);
        }))
    {
      fail(h, "unusable", "the pattern does not mention " + nameOf(q[0][j]));
      return;
    }
  }
  auto t = std::make_shared<inst::Trigger>(
      d_envRef, d_qstate, d_qim, d_qreg, d_treg, q, nodes, true);
  t->setInferenceId(InferenceId::QUANTIFIERS_INST_LLM_DIRECTED);
  d_triggers.push_back({i, t});
  h.d_status = "applied";
  h.d_reason.clear();
  if (h.d_patternShown.empty())
  {
    h.d_patternShown = shown;
  }
  NodeManager* nm = nodeManager();
  Node ipl = nm->mkNode(Kind::INST_PATTERN_LIST,
                        nm->mkNode(Kind::INST_PATTERN, shown));
  Node materialized = nm->mkNode(Kind::FORALL, q[0], q[1], ipl);
  if (std::find(h.d_materialized.begin(), h.d_materialized.end(), materialized)
      == h.d_materialized.end())
  {
    h.d_materialized.push_back(materialized);
  }
}

bool Speculation::isBlocked(const Node& q,
                            const std::vector<Node>& terms,
                            const Node& trigger)
{
  if (d_directing || d_hyps.size() == 0)
  {
    return false;
  }
  std::vector<Node> candidates;
  for (size_t i = 0, n = d_hyps.size(); i < n; i++)
  {
    const std::shared_ptr<Hypothesis>& h = d_hyps[i];
    if (h->d_req.d_kind != SpeculationRequest::Kind::BLOCK
        || qidOf(q) != h->d_req.d_qid)
    {
      continue;
    }
    // The bindings, and the trigger instance the match was made on, each in
    // original form: the fingerprint is written against what the matching
    // loop report prints.
    if (candidates.empty())
    {
      for (const Node& t : terms)
      {
        candidates.push_back(SkolemManager::getOriginalForm(t));
      }
      if (!trigger.isNull() && trigger.getKind() == Kind::SEXPR)
      {
        std::vector<Node> vars(q[0].begin(), q[0].end());
        for (const Node& p : trigger)
        {
          candidates.push_back(SkolemManager::getOriginalForm(p.substitute(
              vars.begin(), vars.end(), terms.begin(), terms.end())));
        }
      }
    }
    for (const Node& c : candidates)
    {
      std::map<std::string, Node> holes;
      if (matches(c, h->d_fp, holes))
      {
        h->d_blocked++;
        d_blockedThisCheck = true;
        keep(*h, terms);
        return true;
      }
    }
  }
  return false;
}

void Speculation::notifyAdded(const Node& q,
                              const std::vector<Node>& terms,
                              InferenceId id,
                              uint64_t round)
{
  if (d_hyps.size() == 0)
  {
    return;
  }
  bool directed = id == InferenceId::QUANTIFIERS_INST_LLM_DIRECTED;
  if (directed && d_current != nullptr)
  {
    d_current->d_added++;
    keep(*d_current, terms);
  }
  uint64_t depth = 0;
  for (const Node& t : terms)
  {
    depth = std::max(depth,
                     static_cast<uint64_t>(TermUtil::getTermDepth(
                         SkolemManager::getOriginalForm(t))));
  }
  Depth& d = d_depth[q];
  d.d_directed += directed ? 1 : 0;
  if (d.d_count++ == 0)
  {
    d.d_first = d.d_max = depth;
    d.d_firstRound = d.d_lastRound = round;
    d.d_rounds = 1;
    return;
  }
  if (round != d.d_lastRound)
  {
    d.d_rounds++;
    d.d_lastRound = round;
  }
  if (depth > d.d_max)
  {
    d.d_max = depth;
    // one rise per round, and none in the round the formula started in,
    // whose terms were all there before it
    if (round != d.d_firstRound && round != d.d_lastRise)
    {
      d.d_rises++;
      d.d_lastRise = round;
    }
  }
}

void Speculation::print(std::ostream& out, uint64_t rounds) const
{
  out << "(:active " << (isActive() ? "true" : "false") << " :rounds " << rounds
      << " :loop-threshold " << d_threshold.get() << " :hypotheses (";
  for (size_t i = 0, n = d_hyps.size(); i < n; i++)
  {
    const Hypothesis& h = *d_hyps[i];
    out << (i == 0 ? "" : " ");
    if (h.d_req.d_kind == SpeculationRequest::Kind::OBSERVE)
    {
      out << "(observe)";
      continue;
    }
    std::string status = h.d_status;
    if (status == "pending" && h.d_quants.empty() && h.d_considered)
    {
      status = "no-quantifier";
    }
    switch (h.d_req.d_kind)
    {
      case SpeculationRequest::Kind::INSTANTIATE: out << "(instantiate"; break;
      case SpeculationRequest::Kind::TRIGGER: out << "(trigger"; break;
      default: out << "(block"; break;
    }
    out << " :qid " << quoteSymbol(h.d_req.d_qid) << " :status " << status;
    if (!h.d_reason.empty())
    {
      out << " :reason " << quoteString(h.d_reason);
    }
    out << " :quantifiers " << h.d_quants.size();
    switch (h.d_req.d_kind)
    {
      case SpeculationRequest::Kind::INSTANTIATE:
        out << " :added " << h.d_added << " :rejected " << h.d_rejected
            << " :instances ";
        printVectors(out, h.d_instances);
        out << " :bodies ";
        printTerms(out, h.d_bodies);
        break;
      case SpeculationRequest::Kind::TRIGGER:
        out << " :pattern ";
        printTerms(out, h.d_patternShown);
        out << " :added " << h.d_added << " :instances ";
        printVectors(out, h.d_instances);
        out << " :materialized ";
        printTerms(out, h.d_materialized);
        break;
      default:
        out << " :fingerprint " << quoteString(h.d_req.d_fingerprint)
            << " :blocked " << h.d_blocked << " :examples ";
        printVectors(out, h.d_instances);
        break;
    }
    out << ")";
  }
  out << ") :loops (";
  std::vector<std::pair<Node, Depth>> loops;
  for (const auto& [q, d] : d_depth)
  {
    if (d.d_rises >= d_threshold.get())
    {
      loops.emplace_back(q, d);
    }
  }
  std::stable_sort(
      loops.begin(), loops.end(), [](const auto& a, const auto& b) {
        return a.second.d_rises > b.second.d_rises;
      });
  for (size_t i = 0, n = loops.size(); i < n; i++)
  {
    const auto& [q, d] = loops[i];
    const std::string& qid = qidOf(q);
    out << (i == 0 ? "" : " ") << "(loop :qid "
        << (qid.empty() ? "_" : quoteSymbol(qid)) << " :instantiations "
        << d.d_count << " :directed " << d.d_directed << " :rounds "
        << d.d_rounds << " :rises " << d.d_rises << " :first-depth "
        << d.d_first << " :max-depth " << d.d_max << " :first-round "
        << d.d_firstRound << " :last-round " << d.d_lastRound << ")";
  }
  out << "))";
}

}  // namespace quantifiers
}  // namespace theory
}  // namespace cvc5::internal
