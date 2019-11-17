// -*- mode:c++; c-basic-offset:2 -*-
// ==================================================================================================
#include <algorithm>
#include <array>
#include <bitset>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <deque>
#include <random>
#include <sstream>
#include <tuple>
#include <unordered_map>
#include <vector>
#include <sys/time.h>

#include "debug_logging.h"

std::random_device RandomDeviceInstance;

class rand_uniform_real_distribution {
public:
  double operator()() {
    return dist(e);
  }
private:
  std::default_random_engine e{RandomDeviceInstance()};
  std::uniform_real_distribution<double> dist;
};

static rand_uniform_real_distribution random_real;

enum class Buff : unsigned char {
  // This is not an in-game buff per se.  But it fits into the definition of Buff well: a temporary
  // status that lasts a fixed amount of steps.
  FirstStep,
  GreatStrides,
  Innovation,
  Manipulation,
  MuscleMemory,

  // Both WasteNot and WasteNotII activate this effect, but with different durations.
  WasteNot,
  Ingenuity,
  Observe,
  FinalAppraisal,

  // Guard.
  NumBuffs,
};

constexpr unsigned char Buff2ID(Buff buff) {
  return static_cast<unsigned char>(buff);
}

enum class Action : unsigned char {
  BasicSynthesis,
  BasicTouch,
  ByregotsBlessing,
  CarefulSynthesis,
  DelicateSynthesis,
  FinalAppraisal,
  FocusedSynthesis,
  FocusedTouch,
  GreatStrides,
  HastyTouch,
  Ingenuity,
  InnerQuiet,
  Innovation,
  IntensiveSynthesis,
  Manipulation,
  MastersMend,
  MuscleMemory,
  Observe,
  PatientTouch,
  PreciseTouch,
  PreparatoryTouch,
  PrudentTouch,
  RapidSynthesis,
  Reflect,
  Reuse,
  StandardTouch,
  TricksOfTheTrade,
  WasteNot,
  WasteNotII,

  // The following actions are not implemented.

  // BrandoftheElements,
  // CarefulObservation,
  // NameoftheElements,
  // TrainedEye,

  // Not an action.
  NumActions,
};

constexpr unsigned char Action2ID(Action ac) {
  return static_cast<unsigned char>(ac);
}

constexpr unsigned TotalActionCount = Action2ID(Action::NumActions);

// All possible crafting conditions.
//
// Condition transfer matrix is listed below.  Note that probabilities for Normal => other
// conditions are estimates copied from internet, I have never verified the numbers.  And it's very
// likely that the transition rate Normal ==> Good is too high.
//
// Normal    ==> Normal(0.75), Good(0.23), Excellent(0.02).
// Good      ==> Normal(1.00),
// Excellent ==> Poor(1.00),
// Poor      ==> Normal(1.00),
enum class Condition : unsigned char {
  Normal,
  Good,
  Excellent,
  Poor,
};

Condition RandomlyGenNextCondition(Condition last_condition) {
  switch (last_condition) {
  case Condition::Normal:
    // 1. Condition Excellent is not simulated: I don't have a good approximation on the probability
    // of transition Normal -> Excellent.  One source quotes the number 0.02, but that looks too
    // high to me.  Even the 0.25 probability of Normal -> Good is likely too high.
    return random_real() > 0.75 ? Condition::Good : Condition::Normal;
  case Condition::Good:
    return Condition::Normal;
  case Condition::Excellent:
    return Condition::Poor;
  case Condition::Poor:
    return Condition::Normal;
  default:
    ASSERT(false) << "Unsupported condition: " << static_cast<unsigned char>(last_condition);
    // Suppress compiler warning if ASSERT() is disabled.
    return Condition::Normal;
  }
}

// This defines a struct to store basic parameters for each action, actions may grant buffs or have
// other effects that are not handled here.
struct ActionParams {
  const char* name;
  char d_cp;
  char d_durability;

  // Percentage value of success change, value range is [0, 100].
  short probability_percentage;
  short efficiency;
  unsigned short flags;
};

enum ActionFlag {
  NONE = 0U,
  PROGRESS = 1U << 0,
  QUALITY = 1U << 1,
};

// The ordering of the items below must match the ordering in AllActions.
const std::array<ActionParams, Action2ID(Action::NumActions) + 1> ALL_ACTIONS{{
    {"BasicSynthesis",       0, -10, 100, 120, PROGRESS},
    {"BasicTouch",         -18, -10, 100, 100, QUALITY},
    {"ByregotsBlessing",   -24, -10, 100, 100, QUALITY},
    {"CarefulSynthesis",    -7, -10, 100, 150, PROGRESS},
    {"DelicateSynthesis",  -32, -10, 100, 100, PROGRESS | QUALITY},
    {"FinalAppraisal",      -1,   0, 100,   0, NONE},
    {"FocusedSynthesis",    -5, -10,  50, 200, PROGRESS},
    {"FocusedTouch",       -18, -10,  50, 150, QUALITY},
    {"GreatStrides",       -32,   0, 100,   0, NONE},
    {"HastyTouch",           0, -10,  60, 100, QUALITY},
    {"Ingenuity",          -22,   0, 100,   0, NONE},
    {"InnerQuiet",         -18,   0, 100,   0, NONE},
    {"Innovation",         -18,   0, 100,   0, NONE},
    {"IntensiveSynthesis",  -6, -10, 100, 300, PROGRESS},
    {"Manipulation",       -96,   0, 100,   0, NONE},
    {"MastersMend",        -88,  30, 100,   0, NONE},
    {"MuscleMemory",        -6, -10, 100, 300, PROGRESS},
    {"Observe",             -7,   0, 100,   0, NONE},
    {"PatientTouch",        -6, -10,  50, 100, QUALITY},
    {"PreciseTouch",       -18, -10, 100, 150, QUALITY},
    {"PreparatoryTouch",   -40, -20, 100, 200, QUALITY},
    {"PrudentTouch"    ,   -25,  -5, 100, 100, QUALITY},
    {"RapidSynthesis",       0, -10,  50, 500, PROGRESS},
    {"Reflect",            -24, -10, 100, 100, QUALITY},
    {"Reuse",              -60,   0, 100,   0, NONE},
    {"StandardTouch",      -32, -10, 100, 125, QUALITY},
    {"TricksOfTheTrade",    20,   0, 100,   0, NONE},
    {"WasteNot",           -56,   0, 100,   0, NONE},
    {"WasteNotII",         -98,   0, 100,   0, NONE},
    {nullptr,                0,   0,   0,   0, NONE},
  }};

const char* Action2Name(Action ac) {
  const char* ptr = ALL_ACTIONS[Action2ID(ac)].name;
  ASSERT(ptr) << "Invalid action " << static_cast<unsigned char>(ac);
  return ptr;
}

//==================================================================================================
struct CraftParams {
public:
  // Character based parameters.  base_craftsmanship is not included as I have not established the
  // formula to use it.
  unsigned short max_cp;
  unsigned short max_durability;
  unsigned short base_control;

  // Recipe based parameters.
  unsigned short max_progress;
  unsigned short max_quality;

  // Eventually, the following should be removed and replaced by formulas.  These are progress gain
  // via specific actions.
  //
  // Progress at efficiency 100%.
  unsigned short base_progress;
  // Same progress while under the effect of Ingenuity.
  unsigned short ig_progress;

  // Coefficient modifier when calculating quality gain.
  double base_quality_coef;
  double ig_quality_coef;
};

// The following data are collected on recipe Grade 2 Tincture of Mind (Level 70 3 stars), with my
// own character's base craftsmapship, control, and CP.
static const CraftParams params = {
  .max_cp = 540,
  .max_durability = 70,
  .base_control = 2079,

  .max_progress = 5645,
  .max_quality = 37432,

  .base_progress = 639,
  .ig_progress = 365,

  .base_quality_coef = 15.5163,
  .ig_quality_coef = 26.3881,
};

//==================================================================================================
class State {
public:
  State():cp(params.max_cp),
          durability(params.max_durability),
          buff({1}) {
    ASSERT(params.max_durability <= 120) << "max_durability won't fit into one byte: " << params.max_durability;
  }

  std::string DebugString() const {
    std::stringstream ss;
    ss << " CP: " << std::setw(3) << cp << "/" << params.max_cp
       << ", DUR: " << std::setw(3) << (unsigned)durability << "/" << params.max_durability
       << ", P: " << std::setw(4) << progress << "/" << params.max_progress
       << ", Q: " << std::setw(5) << quality << "/" << params.max_quality
       << ", I: " << std::setw(2) << static_cast<unsigned>(inner_quiet) << "/" << 11
       << ", C: " << std::setw(1) << static_cast<unsigned>(condition)
       << ", FS: " << std::setw(1) << (unsigned)buff[Buff2ID(Buff::FirstStep)]
       << ", GS: " << std::setw(1) << (unsigned)buff[Buff2ID(Buff::GreatStrides)]
       << ", IN: " << std::setw(1) << (unsigned)buff[Buff2ID(Buff::Innovation)]
       << ", MN: " << std::setw(1) << (unsigned)buff[Buff2ID(Buff::Manipulation)]
       << ", MM: " << std::setw(1) << (unsigned)buff[Buff2ID(Buff::MuscleMemory)]
       << ", WN: " << std::setw(1) << (unsigned)buff[Buff2ID(Buff::WasteNot)]
       << ", IG: " << std::setw(1) << (unsigned)buff[Buff2ID(Buff::Ingenuity)]
       << ", OB: " << std::setw(1) << (unsigned)buff[Buff2ID(Buff::Observe)]
       << ", FA: " << std::setw(1) << (unsigned)buff[Buff2ID(Buff::FinalAppraisal)];
    return ss.str();
  }

  // This provides a simple way to backlist actions that we do not want to include in the
  // simulation.
  bool IsActionSupported(Action ac) const {
    switch (ac) {
    case Action::Reuse:
      return false;
    default:
      return true;
    }
  }

  // Checks if an action can be executed based on buff, inner_quiet, condition, and cp.  Does not
  // check if durability will drop to 0 or negative (which is performed in game after the action is
  // done).  This is basically the same as the in game check.
  bool CanExecuteAction(Action ac) const {
    if (!IsActionSupported(ac)) {
      return false;
    }
    const ActionParams& ac_effect = ALL_ACTIONS[Action2ID(ac)];
    if (cp + ac_effect.d_cp < 0) {
      return false;
    }
    // Action specific checks.
    switch (ac) {
    case Action::TricksOfTheTrade:
    case Action::PreciseTouch:
    case Action::IntensiveSynthesis:
      return condition == Condition::Good || condition == Condition::Excellent;
    case Action::ByregotsBlessing:
      return inner_quiet > 1;
    case Action::FinalAppraisal:
      return buff[Buff2ID(Buff::FinalAppraisal)] == 0;
    case Action::InnerQuiet:
      return inner_quiet == 0;
    case Action::Innovation:
      return buff[Buff2ID(Buff::Innovation)] == 0;
    case Action::MuscleMemory:
    case Action::Reflect:
      return buff[Buff2ID(Buff::FirstStep)] > 0;
    case Action::PrudentTouch:
      return buff[Buff2ID(Buff::WasteNot)] == 0;
    default:
      return true;
    }
  }

  // Compute Progress changes.  This function should only be executed if the action succeeds.
  void ApplyProgressChange(Action ac) {
    const ActionParams& ac_effect = ALL_ACTIONS[Action2ID(ac)];
    if (!(ac_effect.flags & PROGRESS)) {
      return;
    }
    unsigned efficiency = ac_effect.efficiency;
    short d_progress = buff[Buff2ID(Buff::Ingenuity)] > 0 ? params.ig_progress : params.base_progress;
    d_progress *= efficiency / 100.;
    progress += d_progress;

    if (progress < params.max_progress) {
      return;
    }
    if (buff[Buff2ID(Buff::FinalAppraisal)] > 0) {
      progress = params.max_progress - 1;
      buff[Buff2ID(Buff::FinalAppraisal)] = 0;
    } else {
      progress = params.max_progress;
    }
  }

  void ApplyCPDurabilityChange(Action ac) {
    const ActionParams& ac_effect = ALL_ACTIONS[Action2ID(ac)];
    // Apply any CP changes.  Assume that CP check has been done already.
    cp += ac_effect.d_cp;
    if (cp > params.max_cp) {
      cp = params.max_cp;
    }
    ASSERT(cp >= 0) << DebugString() << " " << ac_effect.name;

    // Apply any durability and progress changes.  This needs to take Waste Not or Waste Not II into
    // consideration.  Durability after this action can become negative.
    short d_durability = ac_effect.d_durability;
    if (d_durability == 0) {
      return;
    }
    if (d_durability < 0 && buff[Buff2ID(Buff::WasteNot)] > 0) {
      ASSERT(d_durability % 2 == 0) << static_cast<unsigned char>(ac) << " " << d_durability;
      d_durability /= 2;
    }
    durability += d_durability;
    if (durability > params.max_durability) {
      durability = params.max_durability;
    }
  }

  // Compute:
  // 1. Quality gain from this action.
  // 2. Change inner quiet stack if applicable.
  //
  // This function should only be executed if the action succeeds.
  //
  // Formula:
  // control = base control * inner quiet multiplier
  // f(c) = 1 + c^2 / 100 + c^4 / 10000
  // quality gain = action efficiency
  //              * buff multiplier (GreatStrides, Innovation)
  //              * condition multiplier
  //              * ingenuity multiplier
  //              * f(control)
  void ApplyQualityChange(Action ac) {
    const ActionParams& ac_effect = ALL_ACTIONS[Action2ID(ac)];
    if (!(ac_effect.flags & QUALITY)) {
      return;
    }
    double efficiency;
    if (ac == Action::ByregotsBlessing) {
      ASSERT(inner_quiet > 1) << inner_quiet;
      efficiency = 1.0 + .2 * (inner_quiet - 1);
    } else {
      efficiency = ac_effect.efficiency / 100.;
    }
    double buff_multiplier = 1.;
    if (buff[Buff2ID(Buff::GreatStrides)] > 0) {
      buff_multiplier += 1.;
    }
    if (buff[Buff2ID(Buff::Innovation)] > 0) {
      buff_multiplier += .2;
    }
    efficiency *= buff_multiplier;

    switch (condition) {
    case Condition::Good:      efficiency *= 1.5; break;
    case Condition::Excellent: efficiency *= 4.0; break;
    case Condition::Poor:      efficiency *= 0.5; break;
    default:
      break;
    }

    // Following is a crude formula from fitted data.  It does not take character and recipe level
    // into consideration, which is why base_quality_coef and ig_quality_coef are required.
    double control = params.base_control;
    if (inner_quiet > 1) {
      control *= 1. + 0.2 * (inner_quiet - 1);
    }
    if (control > params.base_control + 3000) {
      control = params.base_control + 3000;
    }
    double coef = buff[Buff2ID(Buff::Ingenuity)] > 0 ? params.ig_quality_coef : params.base_quality_coef;
    quality += efficiency * coef * (1. + 0.01 * control * (1. + 0.0001 * control));
    if (quality > params.max_quality) {
      quality = params.max_quality;
    }
  }

  // Apply any changes to the number of InnerQuiet stacks.  This function should only be executed if
  // the action succeeds.
  void ApplyInnerQuietChange(Action ac, bool is_action_successful) {
    if (inner_quiet > 0) {
      if (is_action_successful) {
        switch (ac) {
        case Action::BasicTouch:
        case Action::DelicateSynthesis:
        case Action::FocusedTouch:
        case Action::HastyTouch:
        case Action::PrudentTouch:
        case Action::StandardTouch:
          inner_quiet += 1;
          break;
        case Action::PreparatoryTouch:
        case Action::PreciseTouch:
          inner_quiet += 2;
          break;
        case Action::ByregotsBlessing:
          inner_quiet = 0;
          break;
        case Action::PatientTouch:
          inner_quiet *= 2;
          break;
        case Action::InnerQuiet:
          ASSERT(false) << DebugString();
          break;
        default:
          break;
        }
        if (inner_quiet > 11) {
          inner_quiet = 11;
        }
      } else if (ac == Action::PatientTouch) {
        inner_quiet = (inner_quiet + 1) / 2;
      }
    } else if (ac == Action::InnerQuiet) {
      inner_quiet = 1;
    } else if (ac == Action::Reflect) {
      inner_quiet = 3;
    }
  }

  // Apply any benefit effects from active buffs, decrease turn counters for active buffs.
  //
  // Note that:
  //
  // 1. Overflow of CP and durability must be checked.
  // 2. There is no CP gain if the current action is ComfortZone.
  // 3. There is no durability gain if the current action is manipulation.
  void ApplyPersistentBuffEffect(Action ac) {
    if (buff[Buff2ID(Buff::Manipulation)] > 0 && ac != Action::Manipulation) {
      durability += 5;
      if (durability > params.max_durability) {
        durability = params.max_durability;
      }
    }
    // Decrease all active buff counters by 1.
    for (auto& b : buff) {
      if (b > 0) --b;
    }
  }

  // If the action grants a buff, set the buff counter to the corresponding number of turns granted
  // by the action.  Note that some action changes multiple counters.  For example, SteadyHand and
  // SteadHandyII are mutually exclusive.  WasteNot and WasteNotII are technically different buffs
  // in game and are also mutually exclusive, but their effects are the same, so we use the same
  // buff slot for both.
  //
  // This function should only be executed if the action succeeds.
  void ApplyBuffChange(Action ac) {
    switch (ac) {
    case Action::DelicateSynthesis:
      buff[Buff2ID(Buff::MuscleMemory)] = 0;
      buff[Buff2ID(Buff::GreatStrides)] = 0;
      break;
    case Action::BasicTouch:
    case Action::ByregotsBlessing:
    case Action::FocusedTouch:
    case Action::HastyTouch:
    case Action::PatientTouch:
    case Action::PreciseTouch:
    case Action::PreparatoryTouch:
    case Action::PrudentTouch:
    case Action::StandardTouch:
      buff[Buff2ID(Buff::GreatStrides)] = 0; break;
    case Action::GreatStrides:
      buff[Buff2ID(Buff::GreatStrides)] = 3; break;
    case Action::Innovation:
      buff[Buff2ID(Buff::Innovation)] = 4; break;
    case Action::Manipulation:
      buff[Buff2ID(Buff::Manipulation)] = 8; break;
    case Action::BasicSynthesis:
    case Action::CarefulSynthesis:
    case Action::FocusedSynthesis:
    case Action::IntensiveSynthesis:
    case Action::RapidSynthesis:
      buff[Buff2ID(Buff::MuscleMemory)] = 0; break;
    case Action::MuscleMemory:
      buff[Buff2ID(Buff::MuscleMemory)] = 5; break;
    case Action::WasteNot:
      buff[Buff2ID(Buff::WasteNot)] = 4; break;
    case Action::WasteNotII:
      buff[Buff2ID(Buff::WasteNot)] = 8; break;
    case Action::Ingenuity:
      buff[Buff2ID(Buff::Ingenuity)] = 5; break;
    case Action::Observe:
      buff[Buff2ID(Buff::Observe)] = 1; break;
    case Action::FinalAppraisal:
      // Reset of FinalAppraisal is done when it's consumed.
      buff[Buff2ID(Buff::FinalAppraisal)] = 5; break;
    default:
      break;
    }
  }

  bool CheckConditionTransition(Condition next_condition) const {
    switch (condition) {
    case Condition::Normal:
      return next_condition != Condition::Poor;
    case Condition::Good:
      return next_condition == Condition::Normal;
    case Condition::Excellent:
      return next_condition == Condition::Poor;
    case Condition::Poor:
      return next_condition == Condition::Normal;
    default:
      ASSERT(false) << "Unsupported condition: " << static_cast<unsigned>(condition);
      return false;
    }
  }

  // Requires !done().
  void Check(void) const {
    ASSERT(cp >= 0 && cp <= params.max_cp) << DebugString();
    ASSERT(durability > 0 && durability <= params.max_durability) << DebugString();
    ASSERT(progress < params.max_progress) << DebugString();
    ASSERT(inner_quiet >= 0 && inner_quiet <= 11) << DebugString();
  }

  // Compute the probability that this action will succeed.  Return the percentage number, i.e., if
  // the probability is 100% then the returned value is 100.
  short SuccessPercentage(Action ac) const {
    if ((ac == Action::FocusedSynthesis || ac == Action::FocusedTouch)
        && buff[Buff2ID(Buff::Observe)] > 0) {
      return 100;
    }
    return ALL_ACTIONS[Action2ID(ac)].probability_percentage;
  }

  // Whether this craft has finished.  Most other methods in this class requires !done().  Both
  // succeess and failure are counted as finished.
  bool done(void) const {
    return progress >= params.max_progress || durability <= 0;
  }

  bool successful(void) const {
    ASSERT(done()) << DebugString();
    return progress >= params.max_progress;
  }

  double hq_probability(void) const {
    // Fitted function based on data obtained from game.  Error is <= 0.03.
    const double x = (double)quality / params.max_quality;
    return 3.93248 - 4.5301*x + 0.0880231*x*x
      + 0.0780783*log(1 + exp( 58.9631*(x-0.701304)))
      -  0.100762*log(1 + exp(-47.4347*(x-0.821754)))
      +  0.102035*log(1 + exp( 25.2665*(x-0.962651)));
  }

  // Requires done() == true.
  double score(void) const {
    ASSERT(done()) << DebugString();
    return progress >= params.max_progress ? (double)quality / params.max_quality : 0.;
    // return progress >= params.max_progress ? hq_probability() : 0.;
  }

  // Executes a single action, with predetermined random factors from input parameters.
  //
  // Returns true if the action is allowed in game, false otherwise.  Note that this function
  // returns true even if an action botches the synthesis (i.e., durability becomes <= 0 without
  // 100% progress), if the game otherwise allows this action.
  bool DeterministicExecuteAction(Action ac, bool is_action_successful, Condition next_condition) {
    Check();
    ASSERT(!done()) << DebugString();
    ASSERT(SuccessPercentage(ac) < 100 || is_action_successful)
      << "Action can not fail: " << ALL_ACTIONS[Action2ID(ac)].name;
    ASSERT(CheckConditionTransition(next_condition))
      << "Impossible condition transition: " << DebugString() << " " << static_cast<unsigned>(next_condition);
    if (CanExecuteAction(ac)) {
      // Apply any effect in cp, durability, progress, quality, and inner quiet stack.  Change in cp
      // and durability is always carried out, while others only when the action is successful.
      ApplyCPDurabilityChange(ac);
      if (is_action_successful) {
        ApplyProgressChange(ac);
        ApplyQualityChange(ac);
      }
      ApplyInnerQuietChange(ac, is_action_successful);
      ApplyPersistentBuffEffect(ac);
      if (is_action_successful) {
        ApplyBuffChange(ac);
      }
      condition = next_condition;
      return true;
    } else {
      return false;
    }
  }

  // Execute a single action with builtin randomness.
  bool ExecuteAction(Action ac) {
    const short percentage = SuccessPercentage(ac);
    const bool is_action_successful = percentage >= 100 ? true : random_real() * 100 < percentage;
    Condition next_condition = RandomlyGenNextCondition(condition);
    return DeterministicExecuteAction(ac, is_action_successful, next_condition);
  }

  bool operator==(const State& s) const {
    if (done() || s.done()) {
      return done() && s.done() ? quality == s.quality : false;
    }
    // Otherwise, none of the 2 states are finished.
    if (cp != s.cp) return false;
    if (progress != s.progress) return false;
    if (quality != s.quality) return false;
    if (durability != s.durability) return false;
    if (inner_quiet != s.inner_quiet) return false;
    if (condition != s.condition) return false;
    for (unsigned i = 0; i < Buff2ID(Buff::NumBuffs); ++i) {
      if (buff[i] != s.buff[i]) return false;
    }
    return true;
  }

  bool operator!=(const State& s) const {
    return !(*this == s);
  }

  // The following functions are related to feeding a State to a neural network.
  static constexpr size_t size() {
    return 6 + Buff2ID(Buff::NumBuffs);
  }

  std::array<double, 6 + Buff2ID(Buff::NumBuffs)> ConvertToDouble(void) const {
    std::array<double, 6 + Buff2ID(Buff::NumBuffs)> ret{
      (double)cp,
      (double)progress,
      (double)quality,
      (double)durability,
      (double)inner_quiet,
      (double)condition,
    };
    for (size_t i = 0; i < Buff2ID(Buff::NumBuffs); ++i) {
      ret[6 + i] = (double)buff[i];
    }
    return ret;
  }
public:
  short cp = 0;
  short progress = 0;
  short quality = 0;
  short durability = 0;

  // Value definition for inner_quiet:
  // 0: Inactive,
  // 1: Active, but the effect is the same as 0.
  // 2-11: Active with 1-10 stacks.
  //
  // InnerQuiet can not be treated as a buff, since it gains stacks instead loses stacks as crafting
  // progresses.  Also, it gains stacks upon a successful quality action, instead of in each turn.
  unsigned char inner_quiet = 0;

  Condition condition = Condition::Normal;
  std::array<unsigned char, Buff2ID(Buff::NumBuffs)> buff;
};

namespace std {
template<>
struct hash<class State> {
  std::size_t operator()(const State &s)const {
    static_assert(sizeof(size_t) >= sizeof(uint64_t), "Unsupported, size_t too small.");
    // Requires:
    //
    // 0 <= s.cp < 1024,
    // 0 <= s.progress < 8192,
    // 0 <= s.qualaity < 65536,
    // 0 <= s.durability < 160 && s.durability % 5 == 0,
    // 0 <= s.inner_quiet < 16,
    // 0 <= s.condition < 4,
    // and corresponding requirements for buffs.
    ASSERT(params.max_cp < (1ULL << 10)) << params.max_cp;
    ASSERT(params.max_durability < (1ULL << 5) * 5) << params.max_durability;
    ASSERT(params.max_progress < (1ULL << 13)) << params.max_progress;
    ASSERT(params.max_quality < (1ULL << 16)) << params.max_quality;
    ASSERT(s.durability % 5 == 0) << s.durability;

    uint64_t ret1 = s.cp;  // 10 bits
    ret1 = (ret1 << 13) + s.progress;
    ret1 = (ret1 << 16) + s.quality;
    ret1 = (ret1 << 5) + (s.durability < 0 ? 0 : s.durability / 5);
    ret1 = (ret1 << 4) + s.inner_quiet;
    uint64_t ret2 = static_cast<unsigned char>(s.condition);  // 2 bits
    ret2 = (ret2 << 1) + s.buff[Buff2ID(Buff::FirstStep)];
    ret2 = (ret2 << 2) + s.buff[Buff2ID(Buff::GreatStrides)];
    ret2 = (ret2 << 3) + s.buff[Buff2ID(Buff::Innovation)];
    ret2 = (ret2 << 4) + s.buff[Buff2ID(Buff::Manipulation)];
    ret2 = (ret2 << 3) + s.buff[Buff2ID(Buff::MuscleMemory)];
    ret2 = (ret2 << 4) + s.buff[Buff2ID(Buff::WasteNot)];
    ret2 = (ret2 << 3) + s.buff[Buff2ID(Buff::Ingenuity)];
    ret2 = (ret2 << 1) + s.buff[Buff2ID(Buff::Observe)];
    ret2 = (ret2 << 3) + s.buff[Buff2ID(Buff::FinalAppraisal)];
    // Bit budget:
    // ret1: 48 bits used.
    // ret2: 26 bits used.

    // floor(ret1 * <random 128bit integer> / 2^64) mod 2^64 + ret2
    constexpr unsigned __int128 p1 = 0xd25807388964a537ULL;
    constexpr uint64_t p2 = 0x8da1685a49e0891dULL;
    return p2 * ret1 + (uint64_t)(p1 * ret1 >> 64) + ret2;
  }
};
}

// End of Game simulation engine.
//==================================================================================================
// Simple neural network implementation.

namespace mlp {
class Node;
class Edge {
public:
  Edge(size_t size):v(size),dv(size) {}

  double& operator()(size_t id) {
    return v[id];
  }
  const double& operator()(size_t id) const {
    return v[id];
  }
  double& D(size_t id) {
    return dv[id];
  }
  const double& D(size_t id) const {
    return dv[id];
  }
  size_t size(void) const {
    return v.size();
  }
private:
  std::vector<double> v;
  std::vector<double> dv;
};

class Node {
public:
  Node(Edge &x, Edge &y):x(x), y(y) {
  }
  virtual void forward(void) = 0;
  virtual void backward_propagate(double) = 0;
  virtual ~Node() {}
protected:
  Edge &x, &y;
};

class ReLU : public Node {
public:
  ReLU(Edge &x, Edge &y):Node(x, y) {
    ASSERT(x.size() == y.size()) << x.size() << " " << y.size();
  }
  void forward(void) override final {
    for (size_t i = 0; i < x.size(); ++i) {
      y(i) = x(i) > 0 ? x(i) : a * x(i);
    }
  }
  void backward_propagate(double) override final {
    for (size_t i = 0; i < x.size(); ++i) {
      x.D(i) = y(i) > 0 ? y.D(i) : a * y.D(i);
    }
  }
private:
  const double a = 0.01;
};

class AffineMap : public Node {
public:
  AffineMap(Edge& x, Edge& y)
    : Node(x, y), w(x.size() * y.size()), b(y.size()) {
    for (auto& wi : w) {
      wi = 0.001 * (random_real() - 0.5);
    }
    for (auto& bi : b) {
      bi = 0.001 * (random_real() - 0.5);
    }
  }
  void forward(void) override final {
    for (size_t i = 0; i < y.size(); ++i) {
      y(i) = b[i];
      for (size_t j = 0; j < x.size(); ++j) {
        y(i) += w[i * x.size() + j] * x(j);
      }
    }
  }
  void backward_propagate(double step_size) override final {
    for (size_t j = 0; j < x.size(); ++j) {
      x.D(j) = 0.0;
    }
    for (size_t i = 0; i < y.size(); ++i) {
      for (size_t j = 0; j < x.size(); ++j) {
        x.D(j) += y.D(i) * w[i * x.size() + j];
        w[i * x.size() + j] = w[i * x.size() + j] * (1. - 2. * weight_decay * step_size) - step_size * y.D(i) * x(j);
      }
      b[i] = b[i] * (1. - 2. * weight_decay * step_size) - step_size * y.D(i);
    }
  }
private:
  std::vector<double> w;
  std::vector<double> b;
  static constexpr double weight_decay = 0.002;
};

// This class simply bundles a SoftMax and a single scalar sigmoid (the last element in the output
// edge), used to predict a set of probabilities (summing to 1) and a scalar value.
class SoftMaxAndSigmoid : public Node {
public:
  SoftMaxAndSigmoid(Edge& x, Edge& y): Node(x, y), size(y.size() - 1) {
    ASSERT(y.size() >= 3) << y.size();
    ASSERT(x.size() == y.size()) << x.size() << " " << y.size();
  }

  void forward(void) override final {
    // SoftMax
    double xmax = x(0);
    for (size_t i = 1; i < size; ++i) {
      xmax = std::max(xmax, x(i));
    }
    ASSERT(std::fabs(xmax) < 1.e100) << xmax;

    double sum = 0.;
    for (size_t i = 0; i < size; ++i) {
      y(i) = std::exp(x(i) - xmax);
      sum += y(i);
    }
    ASSERT(sum >= 1.0 && sum <= 1.01 * size) << sum;

    sum = 1.0 / sum;
    for (size_t i = 0; i < size; ++i) {
      y(i) *= sum;
    }

    // Sigmoid
    y(size) = 1. / (1. + std::exp(bias - x(size)));
  }

  void backward_propagate(double) override final {
    // SoftMax
    double sum = 0.;
    for (size_t i = 0; i < size; ++i) {
      x.D(i) = y(i) * y.D(i);
      sum += x.D(i);
    }
    for (size_t i = 0; i < size; ++i) {
      x.D(i) -= y(i) * sum;
    }

    // Sigmoid
    x.D(size) = y.D(size) * y(size) * (1. - y(size));
  }
private:
  // This is so that initially the Sigmoid outputs a value close to 0.
  static constexpr double bias = 10.;
  const size_t size;
};

class MLP {
public:
  MLP(const std::vector<size_t>& hidden_layer_sizes) {
    const size_t layers = hidden_layer_sizes.size() + 1;

    e.emplace_back(State::size());
    for (size_t size : hidden_layer_sizes) {
      e.emplace_back(size);
      e.emplace_back(size);
    }
    // Output layer is a SoftMax function that associate each move with a probability,
    // plus a scalar value that predicts the score.
    e.emplace_back(Action2ID(Action::NumActions) + 1);
    e.emplace_back(Action2ID(Action::NumActions) + 1);

    for (size_t i = 0; i < layers; ++i) {
      v.emplace_back(std::make_unique<AffineMap>(e[2 * i], e[2 * i + 1]));
      if (i + 1 < layers) {
        v.emplace_back(std::make_unique<ReLU>(e[2 * i + 1], e[2 * i + 2]));
      } else {
        v.emplace_back(std::make_unique<SoftMaxAndSigmoid>(e[2 * i + 1], e[2 * i + 2]));
      }
    }
  }

  const Edge& forward(const State& s) {
    const auto in(s.ConvertToDouble());
    ASSERT(e[0].size() == in.size()) << e[0].size() << " " << in.size();
    for (size_t i = 0; i < in.size(); ++i) {
      e[0](i) = in[i];
    }
    for (size_t i = 0; i < v.size(); ++i) {
      v[i]->forward();
    }
    return e.back();
  }

  // Training data are (State, vector of probability, score) tuple.  This function only supports training
  // with one example at a time.
  void train(const State& in, const std::array<double, TotalActionCount>& p, double score, double step_size, bool track_simulation) {
    {
      double total = 0.;
      for (size_t i = 0; i < TotalActionCount; ++i) {
        total += p[i];
      }
      ASSERT(fabs(total - 1.) < 1e-10) << in.DebugString() << ", total = " << total;
    }

    // cost function is MLE + weight decay.
    forward(in);

    Edge &eb = e.back();
    const size_t size = eb.size() - 1;
    ASSERT(p.size() == size) << p.size() << " != " << size;
    // For probabilities, use max likelihood loss function.
    for (size_t i = 0; i < size; ++i) {
      eb.D(i) = -p[i] / (1e-10 + eb(i));
    }
    // For score, we use mean square root as error function.
    // eb.D(size) = 2. * (eb(size) - score);

    // Or this loss function (which proves to be much better than the above since we want the
    // sigmoid to output a near-zero score initially):
    
    // l = (log(z/((1-e)*s+e)))^2
    // Where e is a very small positive number (to eliminate the singularity), z = sigmoid output, s = score.
    double s = (1 - 1e-5) * score + 1e-5;
    eb.D(size) = 2 * std::log(eb(size)/s) / eb(size);

    for (size_t i = v.size(); i > 0; --i) {
      v[i - 1]->backward_propagate(step_size);
    }

    if (track_simulation) {
      std::cout << "MLP training: " << in.DebugString() << " ==>\n";
      for (size_t i = 0; i < size; ++i) {
        std::cout << "MLP training: " << std::setfill(' ') << std::setw(20) << Action2Name(static_cast<Action>(i))
                  << std::setw(14) << std::scientific << std::setprecision(3) << p[i]
                  << std::setw(14) << std::scientific << std::setprecision(3) << eb(i)
                  << std::setw(14) << std::scientific << std::setprecision(3) << eb.D(i) << "\n";
      }
      std::cout << "MLP training: " << std::setw(20) << "<score>:"
                << std::setw(14) << std::scientific << std::setprecision(3) << score
                << std::setw(14) << std::scientific << std::setprecision(3) << eb(size)
                << std::setw(14) << std::scientific << std::setprecision(3) << eb.D(size) << "\n";
    }
  }
private:
  std::vector<std::unique_ptr<Node>> v;
  std::vector<Edge> e;
};
}  // namespace mlp

//==================================================================================================

// This class generates random vectors following Dirichlet distribution (with uniform exponent) for
// use as a noise.
template<size_t N>
class DirichletDist {
public:
  DirichletDist(double c)
    : e(RandomDeviceInstance())
    , gamma(c, 1.) {}

  const std::array<double, N>& gen() {
    double sum = 0.;
    for (size_t i = 0; i < N; ++i) {
      x[i] = gamma(e);
      sum += x[i];
    }
    sum = 1. / sum;
    for (size_t i = 0; i < N; ++i) {
      x[i] *= sum;
    }
    return x;
  }
private:
  std::array<double, N> x;

  std::default_random_engine e;
  std::gamma_distribution<double> gamma;
};

class UCT {
public:
  struct StateStatistics {
    StateStatistics()
      : total_count(0) {
      ac_valid.set();
    }

    std::string DebugString() {
      std::stringstream ss;
      for (size_t id = 0; id < TotalActionCount; ++id) {
        Action ac = static_cast<Action>(id);
        if (!valid(ac)) continue;
        double v = count(ac) == 0 ? 0. : value(ac) / count(ac);
        ss << "       " << std::setw(20) << std::setfill(' ') << Action2Name(ac)
           << ", prior = " << std::setfill('0') << std::fixed << prior(ac)
           << ", visit = " << std::setw(10) << std::setfill(' ') << count(ac)
           << ", value = " << std::setw(14) << std::setfill(' ') << std::scientific << v << "\n";
      }
      return ss.str();
    }

    void setValid(Action ac, bool v) {
      size_t id = Action2ID(ac);
      ASSERT(id < Action2ID(Action::NumActions)) << Action2Name(ac);
      ac_valid[id] = v;
    }
    bool valid(Action ac) const {
      size_t id = Action2ID(ac);
      ASSERT(id < Action2ID(Action::NumActions)) << Action2Name(ac);
      return ac_valid[id];
    }
    double& prior(Action ac) {
      ASSERT(valid(ac)) << DebugString() << ", ac = " << Action2Name(ac);
      size_t id = Action2ID(ac);
      return std::get<0>(ac_stat[id]);
    }
    uint64_t& count(Action ac) {
      ASSERT(valid(ac)) << DebugString() << ", ac = " << Action2Name(ac);
      size_t id = Action2ID(ac);
      return std::get<1>(ac_stat[id]);
    }
    double& value(Action ac) {
      ASSERT(valid(ac)) << DebugString() << ", ac = " << Action2Name(ac);
      size_t id = Action2ID(ac);
      return std::get<2>(ac_stat[id]);
    }

    // Storage format: For each possible action starting from this position, the following is stored:
    // 1. Prior probability, generated by neural network.
    // 2. Visit count.
    // 3. Action value.
    std::array<std::tuple<double, uint64_t, double>, TotalActionCount> ac_stat;

    // This is the total play out count from this state.
    uint64_t total_count;
    // true ==> this action is valid (false: either the action not allowed in game with this state,
    // or it immediately leads to failure, e.g., durability drops to 0 before it finishes).
    std::bitset<TotalActionCount> ac_valid;
  };

  UCT(mlp::MLP& _scn, const State& root)
    : dir(1.03), scn(_scn)
  {
    reset(root);
  }

  double initState(const State& s) {
    ASSERT(all_states.find(s) == all_states.end()) << s.DebugString();

    StateStatistics stat;
    const mlp::Edge& edge = scn.forward(s);
    const std::array<double, TotalActionCount>& noise = dir.gen();
    const double eps = 0.25;
    for (size_t ac_id = 0; ac_id < TotalActionCount; ++ac_id) {
      Action ac = static_cast<Action>(ac_id);
      stat.prior(ac) = edge(ac_id) * (1. - eps) + eps * noise[ac_id];
      stat.count(ac) = 0;
      stat.value(ac) = 0.;
      if (!s.CanExecuteAction(ac)) {
        stat.setValid(ac, false);
      }
    }
    all_states.emplace(s, stat);
    return edge(TotalActionCount);
  }

  void reset(const State& root) {
    all_states.clear();
    initState(root);
  }

  double simulateFromState(const State& s, bool track_simulation) {
    if (s.done()) {
      LOG(track_simulation) << s.DebugString() << ": (UCT)==> done " << s.score()
          << (s.successful() ? " <Finished>." : " <Failed>.");
      return s.score();
    }
    if (all_states.find(s) == all_states.end()) {
      double score = initState(s);
      LOG(track_simulation) << s.DebugString() << "\n(UCT)==> NN estimation = "
                            << std::scientific << std::setprecision(3) << score << ".";
      return score;
    }
    StateStatistics& stat = all_states.at(s);
    LOG(track_simulation) << s.DebugString() << "\n" << stat.DebugString();

    double ucb1_max = -std::numeric_limits<double>::infinity();
    Action ac_max = Action::NumActions;
    double nsq = std::sqrt((double)stat.total_count);
    for (size_t ac_id = 0; ac_id < TotalActionCount; ++ac_id) {
      Action ac = static_cast<Action>(ac_id);
      if(!stat.valid(ac)) continue;
      double u = stat.count(ac) == 0 ? 0. : stat.value(ac) / stat.count(ac);
      u += stat.prior(ac) * nsq / (1 + stat.count(ac));
      if (u > ucb1_max) {
        ucb1_max = u;
        ac_max = ac;
      }
    }
    if (ac_max == Action::NumActions) {
      LOG(track_simulation) << "(UCT)==> <Failed>.";
      ++stat.total_count;
      return 0.;
    } else {
      LOG(track_simulation) << "(UCT)==> picked " << Action2Name(ac_max) << ".";
    }

    State next(s);
    next.ExecuteAction(ac_max);
    double score = simulateFromState(next, track_simulation);
    stat.value(ac_max) += score;
    ++stat.count(ac_max);
    ++stat.total_count;
    return score;
  }

  Action select(const State& s, double inv_temp) {
    ASSERT(all_states.find(s) != all_states.end()) << s.DebugString();
    std::array<double, TotalActionCount> p;

    double sum = 0.;
    StateStatistics& stat = all_states.at(s);
    for (size_t ac_id = 0; ac_id < TotalActionCount; ++ac_id) {
      Action ac = static_cast<Action>(ac_id);
      if (stat.valid(ac)) {
        p[ac_id] = std::pow(stat.count(ac), inv_temp);
        sum += p[ac_id];
      } else {
        p[ac_id] = 0.;
      }
    }
    if (sum == 0.) {
      return Action::NumActions; // Resign.
    }
    double r = random_real() * sum;
    for (size_t ac_id = 0; ac_id < TotalActionCount; ++ac_id) {
      Action ac = static_cast<Action>(ac_id);
      if (!stat.valid(ac)) continue;
      r -= p[ac_id];
      if (r < 0.) {
        return ac;
      }
    }
    ASSERT(false) << s.DebugString() << " " << stat.DebugString();
    return Action::NumActions;
  }

  void set_target_probability(const State& s, double inv_temp,
                              std::array<double, TotalActionCount>& p) {
    ASSERT(!s.done()) << s.DebugString();
    ASSERT(all_states.find(s) != all_states.end()) << s.DebugString();
    StateStatistics& stat = all_states.at(s);
    ASSERT(stat.total_count > 0) << s.DebugString() << " " << stat.DebugString();
    double sum = 0.;
    for (size_t ac_id = 0; ac_id < TotalActionCount; ++ac_id) {
      Action ac = static_cast<Action>(ac_id);
      if (stat.valid(ac) && stat.count(ac) > 0) {
        p[ac_id] = std::pow((double)stat.count(ac), inv_temp);
      } else {
        p[ac_id] = 0.1;
      }
      sum += p[ac_id];
    }
    ASSERT(sum >= 1) << s.DebugString() << " " << stat.DebugString();
    sum = 1. / sum;
    for (size_t ac_id = 0; ac_id < TotalActionCount; ++ac_id) {
      p[ac_id] *= sum;
    }
  }
private:
  DirichletDist<TotalActionCount> dir;
  // Neural Network.
  mlp::MLP& scn;
  // Monte Carlo search tree.
  std::unordered_map<State, StateStatistics> all_states;
};

class Driver {
public:
  Driver()
    : scn({(size_t)Action2ID(Action::NumActions) * 2, (size_t)Action2ID(Action::NumActions) * 2})
    , uct(scn, root_state)
  {}

  void simulate() {
    uct.reset(root_state);
    State s(root_state);
    std::vector<State> states;
    double score = 0.;

    const size_t size = training_data.size();
    ++simulate_count;
    bool track_simulation = simulate_count % 16 == 0;
    while(true) {
      if (s.done()) {
        LOG(track_simulation) << "Sample play: done, score = " << std::scientific << std::setprecision(3) << s.score() << "\n";
        score = s.score();
        break;
      }
      training_data.emplace_back(s, std::array<double, TotalActionCount>({}), 0.);
      for (size_t i = 0; i < SimulateCount; ++i) {
        uct.simulateFromState(s, track_simulation && i == SimulateCount - 1);
        // uct.simulateFromState(s, false);
      }
      // Select a move.
      Action ac = uct.select(s, inv_temp);
      LOG(track_simulation) << "Sample play: " << s.DebugString() << " ==> " << Action2Name(ac);
      if (ac == Action::NumActions) {
        LOG(track_simulation) << "Sample play: resigned.\n";
        break;
      } else {
        s.ExecuteAction(ac);
      }
    }

    for (size_t i = 0; i + size < training_data.size(); ++i) {
      auto& example = training_data[size + i];
      State& s = std::get<0>(example);
      uct.set_target_probability(s, inv_temp, std::get<1>(example));
      std::get<2>(example) = score;
    }
    while (training_data.size() > 10000) {
      training_data.pop_front();
    }
    LOG(track_simulation) << "Training data: has " << training_data.size() << " examples.";

    size_t example_id = random_real() * training_data.size();
    const auto& example = training_data[example_id];
    LOG(track_simulation) << "Training data: " << std::get<0>(example).DebugString() << " ==>";
    for (size_t i = 0; i < TotalActionCount; ++i) {
      LOG(track_simulation) << "Training data: " << std::setw(20) << std::setfill(' ')
          << Action2Name(static_cast<Action>(i)) << ": "
          << std::setfill('0') << std::scientific << std::setprecision(3) << std::get<1>(example)[i];
    }
    LOG(track_simulation) << "Training data: " << std::setfill(' ') << std::setw(20) << "Final score:"
        << std::scientific << std::setprecision(3) << std::get<2>(example);
  }

  void train() {
    bool track_simulation = simulate_count % 16 == 0;
    const size_t size = training_data.size();
    for (size_t i = 0; i < 100; ++i) {
      ++train_count;
      size_t id = size * random_real();
      auto& example = training_data[id];
      scn.train(std::get<0>(example), std::get<1>(example), std::get<2>(example), step_size, track_simulation && i == 0);
    }
  }
private:
  static constexpr size_t SimulateCount = 10000;
  static constexpr double inv_temp = 1.5;
  static constexpr double step_size = 0.00001;

  size_t simulate_count = 0;
  size_t train_count = 0;

  static const State root_state;
  mlp::MLP scn;
  UCT uct;
  std::deque<std::tuple<State, std::array<double, TotalActionCount>, double>> training_data;
};

const State Driver::root_state;

int main() {
  Driver driver;
  for (size_t count = 0; ; ++count) {
    driver.simulate();
    driver.train();
  }
  return 0;
}
