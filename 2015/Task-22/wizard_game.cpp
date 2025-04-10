#include <Rcpp.h>
#include <algorithm>
#include <iostream>
#include <limits>
#include <memory>
#include <stack>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace Rcpp;

enum Player {
  BOSS,
  PLAYER
};

struct Stats {
  int my_hp;
  int boss_hp;
  int mana;
  int armor;
  int round;
  int mana_spent;
};

class Spell {
public:
  Spell(const std::string, const std::unordered_map<std::string, int> &);
  void cast(Stats &);
  void apply(Stats &);
  int get_cost() const;
  std::string get_name() const;
  int get_duration() const;
  bool is_buff() const;
  
  friend std::ostream &operator<<(std::ostream &, const Spell &);
  
private:
  std::string name;
  int damage;
  int heal;
  int armor;
  int mana;
  int costs;
  int duration;
};

struct ActiveBuffs {
  std::shared_ptr<Spell> spell;
  int usages;
};

class GameController {
public:
  GameController(Stats, int, 
                 const std::unordered_map<std::string, 
                                          std::unordered_map<std::string, int>> &);
  void apply_buffs();
  void attack();
  void cast(const std::string &);
  void sting();
  bool is_dead(Player) const;
  bool can_cast(const std::string &) const;
  int get_mana_spent() const;
  std::vector<std::string> get_spells() const;
  
private:
  std::vector<ActiveBuffs> buffs;
  std::vector<std::string> spells;
  std::unordered_map<std::string, std::shared_ptr<Spell>> spell_library;
  Stats stats;
  int boss_damage;
  
  bool is_active(const std::string &) const;
};

class Game {
public:
  Game();
  int start_game(int, int, int, int, 
                 const std::unordered_map<std::string, 
                                          std::unordered_map<std::string, int>> &,
                                          bool);
  
private:
  int simulate(GameController, bool);
};

Spell::Spell(const std::string spell_name,
             const std::unordered_map<std::string, int> &stats) : 
  name(spell_name),
  damage(stats.at("damage")),
  heal(stats.at("heal")),
  armor(stats.at("armor")),
  mana(stats.at("mana")),
  costs(stats.at("costs")),
  duration(stats.at("duration")) {
}

void Spell::cast(Stats &stats) {
  stats.mana_spent += costs;
  stats.mana -= costs;
  stats.round++;
}

void Spell::apply(Stats &stats) {
  // 1. boss loses hp
  stats.boss_hp -= damage;
  // 2. 1 heal
  stats.my_hp += heal;
  // 3. armor is set up
  stats.armor = std::max(stats.armor, armor);
  // 4. I get mana
  stats.mana += mana;
}

int Spell::get_cost() const {
  return costs;
}

int Spell::get_duration() const {
  return duration;
}

bool Spell::is_buff() const {
  return duration > 0;
}

std::string Spell::get_name() const {
  return name;
}

std::ostream &operator<<(std::ostream &stream, const Spell &spell) {
  stream << spell.name << " [Damage: " << spell.damage << ", Heal: " << spell.heal;
  stream << ", Armor: " << spell.armor << ", Mana: " << spell.mana << ", Costs: ";
  stream << spell.costs << "]" << " @" << spell.duration;
  return stream;
}

GameController::GameController(Stats start_stats, int damage, 
                               const std::unordered_map<std::string, 
                                                        std::unordered_map<std::string, 
                                                                           int>> &all_spells) :
  stats(start_stats), boss_damage(damage) {
  for (const auto &[name, stats] : all_spells) {
    spell_library[name] = std::make_shared<Spell>(name, stats);
    spells.push_back(name);
  }
}

void GameController::apply_buffs() {
  stats.armor = 0;
  for (auto it = buffs.begin(); it != buffs.end();) {
    if (it->usages > 0) {
      it->spell->apply(stats);
      it->usages--;
      if (it->usages == 0) {
        it = buffs.erase(it);
      } else {
        ++it;
      }
    }
  }
}

void GameController::attack() {
  stats.my_hp -= std::max<int>(boss_damage - stats.armor, 1);
}

void GameController::cast(const std::string &spell_name) {
  auto spell = spell_library[spell_name];
  spell->cast(stats);
  if (spell->is_buff()) {
    buffs.push_back({spell, spell->get_duration()});
  } else {
    spell->apply(stats);
  }
}

void GameController::sting() {
  stats.my_hp--;
}

bool GameController::is_dead(Player who) const {
  if (who == BOSS) {
    return stats.boss_hp <= 0;
  } else {
    return stats.my_hp <= 0;
  }
}

bool GameController::can_cast(const std::string &spell_name) const {
  auto spell = spell_library.at(spell_name);
  return stats.mana >= spell->get_cost() && !is_active(spell_name);
}

bool GameController::is_active(const std::string &name) const {
  return std::any_of(buffs.begin(), buffs.end(), [&](const ActiveBuffs &b) {
    return b.spell->get_name() == name;
  });
}

int GameController::get_mana_spent() const {
  return stats.mana_spent;
}

std::vector<std::string> GameController::get_spells() const {
  return spells;
}

Game::Game() {
}

int Game::start_game(int boss_damage, int boss_hp, int my_hp, int mana, 
                     const std::unordered_map<std::string,
                                              std::unordered_map<std::string, 
                                                                 int>> &spell_library,
                                                                 bool hard_game = false) {
  Stats stats;
  stats.my_hp = my_hp;
  stats.boss_hp = boss_hp;
  stats.mana = mana;
  stats.armor = 0U;
  stats.round = 1U;
  stats.mana_spent = 0U;
  GameController controller(stats, boss_damage, spell_library);
  int result = simulate(controller, hard_game);
  return result;
}

int Game::simulate(GameController start_controller, bool hard_game) {
  struct SimulationState {
    GameController controller;
    size_t next_spell_index = 0;
  };
  std::vector<std::string> spells = start_controller.get_spells();
  std::stack<SimulationState> stack;
  stack.push({start_controller,
             0});
  
  int best_mana = std::numeric_limits<int>::max();
  bool found_solution = false;
  
  while (!stack.empty()) {
    SimulationState &current = stack.top();
    
    if (current.next_spell_index >= spells.size()) {
      stack.pop();
      continue;
    }
    
    const std::string &spell = spells[current.next_spell_index++];
    
    GameController controller_copy = current.controller;
    if (hard_game) {
      controller_copy.sting();
    }
    controller_copy.apply_buffs();
    if (controller_copy.is_dead(BOSS)) {
      found_solution = true;
      best_mana = std::min(best_mana, controller_copy.get_mana_spent());
      continue;
    }
    if (!controller_copy.can_cast(spell)) {
      continue;
    }
    controller_copy.cast(spell);
    controller_copy.apply_buffs();
    if (controller_copy.is_dead(BOSS)) {
      found_solution = true;
      best_mana = std::min(best_mana, controller_copy.get_mana_spent());
      continue;
    }
    controller_copy.attack();
    if (controller_copy.is_dead(PLAYER)) {
      continue;
    }
    if (controller_copy.get_mana_spent() >= best_mana) {
      continue;
    }
    stack.push({controller_copy,
               0});
  }
  return found_solution ? best_mana : -1;
}

// [[Rcpp::export]]
int get_lowest_mana(List stats, DataFrame spells, bool hard_game = false) {
  std::unordered_map<std::string, std::unordered_map<std::string, int>> spell_library;
  std::vector<std::string> names = spells["name"];
  std::vector<std::string> fields = {"damage", "heal", "armor", "mana", "duration", 
                                     "costs"};
  for (int i = 0; i < spells.nrows(); ++i) {
    std::unordered_map<std::string, int> spell_stats;
    for (const std::string &field : fields) {
      spell_stats[field] = as<std::vector<int>>(spells[field])[i];
    }
    spell_library[names[i]] = std::move(spell_stats);
  }
  Game game;
  int result = game.start_game(stats["dmg"], stats["boss_HP"], stats["hp"],
                               stats["mana"], spell_library, hard_game);
  return result;
}