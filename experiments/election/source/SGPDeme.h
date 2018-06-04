#ifndef SGPDEME_H
#define SGPDEME_H

#include <iostream>
#include <algorithm>
#include <functional>
#include "base/Ptr.h"
#include "base/vector.h"
#include "control/Signal.h"
#include "hardware/EventDrivenGP.h"
#include "tools/Random.h"
#include "tools/random_utils.h"

//TODO:
// [ ] Signals
// [ ] Test neighbor network

class SGPDeme {
public:
  static constexpr size_t TAG_WIDTH = 16;
  static constexpr size_t DIR_UP = 0;
  static constexpr size_t DIR_LEFT = 1;
  static constexpr size_t DIR_DOWN = 2;
  static constexpr size_t DIR_RIGHT = 3;
  static constexpr size_t NUM_DIRS = 4;

  using hardware_t = emp::EventDrivenGP_AW<TAG_WIDTH>;
  using grid_t = emp::vector<hardware_t>;
  using program_t = hardware_t::Program;
  using state_t = hardware_t::State;
  using inst_t = hardware_t::inst_t;
  using inst_lib_t = hardware_t::inst_lib_t;
  using event_t = hardware_t::event_t;
  using event_lib_t = hardware_t::event_lib_t;
  using memory_t = hardware_t::memory_t;
  using tag_t = hardware_t::affinity_t;

protected:
  grid_t grid;
  size_t width;
  size_t height;
  emp::vector<size_t> schedule; ///< Utility vector to store order to give each hardware in the deme a CPU cycle on a single deme update.
  emp::vector<size_t> neighbor_lookup;
  emp::Ptr<emp::Random> random;

  emp::Ptr<inst_lib_t> inst_lib;
  emp::Ptr<event_lib_t> event_lib;
  program_t deme_program;

  // Signals!
  // - Reset hardware.
  emp::Signal<void(hardware_t &)> on_hardware_reset_sig;
  emp::Signal<void(hardware_t &)> on_hardware_advance_sig;
  emp::Signal<void(void)> on_deme_single_advance_sig;


  void BuildNeighbors() {
    neighbor_lookup.resize(grid.size()*NUM_DIRS);
    for (size_t i = 0; i < grid.size(); ++i) {
      for (size_t d = 0; d < NUM_DIRS; ++d) {
        neighbor_lookup[GetNeighborIndex(i, d)] = CalcNeighbor(i, d);
      }
    }
  }

  size_t CalcNeighbor(size_t id, size_t dir) const {
    int facing_x = (int)GetLocX(id);
    int facing_y = (int)GetLocY(id);
    switch(dir) {
      case DIR_UP:    facing_y = emp::Mod(facing_y - 1, (int)height); break;
      case DIR_LEFT:  facing_x = emp::Mod(facing_x - 1, (int)width);  break;
      case DIR_RIGHT: facing_x = emp::Mod(facing_x + 1, (int)width);  break;
      case DIR_DOWN:  facing_y = emp::Mod(facing_y + 1, (int)height); break;
      default:
        emp_assert(false, "Bad direction!"); // TODO: put an assert here.
        break;
    }
    return GetID(facing_x, facing_y);
  }

  size_t GetNeighborIndex(size_t id, size_t dir) const {
    return (id*NUM_DIRS) + dir;
  }

public:
  SGPDeme(size_t _w, size_t _h, emp::Ptr<emp::Random> _rnd, emp::Ptr<inst_lib_t> _ilib, emp::Ptr<event_lib_t> _elib)
    : grid(), width(_w), height(_h), schedule(width*height), neighbor_lookup(),
      random(_rnd), inst_lib(_ilib), event_lib(_elib), deme_program(inst_lib)
  {
    // Fill out the grid with hardware.
    for (size_t i = 0; i < width*height; ++i) {
      grid.emplace_back(inst_lib, event_lib, random);
      schedule[i] = i;
    }
    BuildNeighbors(); ///< Build neighbor lookup table.
  }

  ~SGPDeme() {
    grid.clear();
  }

  /// Reset the deme.
  void ResetHardware() {
    deme_program.Clear();
    for (size_t i = 0; i < grid.size(); ++i) {
      schedule[i] = i;
      grid[i].ResetHardware();
      on_hardware_reset_sig.Trigger(grid[i]);
    }
  }

  const program_t & GetProgram() const { return deme_program; }
  size_t GetWidth() const { return width; }
  size_t GetHeight() const { return height; }
  size_t GetSize() const { return grid.size(); }

  /// Get x location in deme grid given hardware loc ID.
  size_t GetLocX(size_t id) const { return id % width; }

  /// Get Y location in deme grid given hardware loc ID.
  size_t GetLocY(size_t id) const { return id / width; }

  /// Get loc ID of hardware given an x, y position.
  size_t GetID(size_t x, size_t y) const { return (y * width) + x; }

  size_t GetNeighborID(size_t id, size_t dir) const {
    // TODO: asserts!
    return neighbor_lookup[GetNeighborIndex(id, dir)];
  }

  hardware_t & GetNeighbor(size_t id, size_t dir) { return GetHardware(neighbor_lookup[GetNeighborIndex(id, dir)]); }
  hardware_t & GetHardware(size_t id) { return grid[id]; }

  emp::SignalKey OnHardwareReset(const std::function<void(hardware_t &)> & fun) { return on_hardware_reset_sig.AddAction(fun); }
  emp::SignalKey OnHardwareAdvance(const std::function<void(hardware_t &)> & fun) { return on_hardware_advance_sig.AddAction(fun); }
  emp::SignalKey OnDemeAdvance(const std::function<void(void)> & fun) { return on_deme_single_advance_sig.AddAction(fun); }


  void SetProgram(const program_t & _germ);
  void SetHardwareMaxCores(size_t max_cores);
  void SetHardwareMaxCallDepth(size_t max_depth);
  void SetHardwareMinBindThresh(double threshold);

  void Advance(size_t i = 1) { for (size_t t = 0; t < i; ++t) SingleAdvance(); }
  void SingleAdvance();

  void PrintState(std::ostream & os=std::cout);
};

void SGPDeme::SetProgram(const program_t & _germ) {
  ResetHardware();                              // Reset deme hardware.
  deme_program = _germ;
  for (size_t i = 0; i < grid.size(); ++i) {
    grid[i].SetProgram(deme_program);           // Update grid[i]'s program.
  }
}

/// Hardware configuration option.
/// Set the maximum number of cores on all hardware units in this deme.
void SGPDeme::SetHardwareMaxCores(size_t max_cores) {
  for (size_t i = 0; i < grid.size(); ++i) {
    grid[i].SetMaxCores(max_cores);
  }
}

/// Hardware configuration option.
/// Set the maximum call depth of all hardware units in this deme.
void SGPDeme::SetHardwareMaxCallDepth(size_t max_depth) {
  for (size_t i = 0; i < grid.size(); ++i) {
    grid[i].SetMaxCallDepth(max_depth);
  }
}

void SGPDeme::SetHardwareMinBindThresh(double threshold) {
  for (size_t i = 0; i < grid.size(); ++i) {
    grid[i].SetMinBindThresh(threshold);
  }
}

void SGPDeme::SingleAdvance() {
  emp::Shuffle(*random, schedule); // Shuffle the schedule.
  on_deme_single_advance_sig.Trigger();
  // Distribute CPU cycles.
  for (size_t i = 0; i < schedule.size(); ++i) {
    on_hardware_advance_sig.Trigger(grid[schedule[i]]);
  }
}

void SGPDeme::PrintState(std::ostream & os) {
  os << "==== DEME STATE ====\n";
  // TODO: signal here, passing os
  for (size_t i = 0; i < grid.size(); ++i) {
    os << "--- Agent @ (" << GetLocX(i) << ", " << GetLocY(i) << ") ---\n";
    grid[i].PrintState(os); os << "\n";
  }
}



#endif
