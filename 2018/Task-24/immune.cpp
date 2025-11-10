#ifndef STANDALONE
#include <Rcpp.h>
using namespace Rcpp;
#else
#include <iostream>
#endif

#include <algorithm>
#include <set>
#include <string>
#include <vector>

struct Army {
    int units;
    int hit_points;
    int attack_damage;
    std::string attack_type;
    int initiative;
    std::set<std::string> weaknesses;
    std::set<std::string> immunities;

    int effective_power() const { return units * attack_damage; }

    int damage_to(const Army& defender) const {
      if (defender.immunities.count(attack_type)) {
        return 0;
      }
      int damage = effective_power();
      if (defender.weaknesses.count(attack_type)) {
        damage *= 2;
      }
      return damage;
    }
};

class Battle {
  public:
    Battle(const std::vector<Army>&, const std::vector<Army>&);
    std::pair<bool, int> fight();
    int find_booster();
    void boost(int);

  private:
    std::vector<Army> immune_system;
    std::vector<Army> infection;
    std::vector<int> target_selection() const;
    bool attack(const std::vector<int>&);
};

Battle::Battle(const std::vector<Army>& immune, const std::vector<Army>& infect)
    : immune_system(immune)
    , infection(infect) { }

void Battle::boost(int amount) {
  for (auto& army : immune_system) {
    army.attack_damage += amount;
  }
}

std::pair<bool, int> Battle::fight() {
  while (!immune_system.empty() && !infection.empty()) {
    auto targets = target_selection();
    if (!attack(targets))
      return std::make_pair(false, 0); // Stalemate
  }
  int total_units = 0;
  const auto& winner = immune_system.empty() ? infection : immune_system;
  for (const auto& army : winner) {
    total_units += army.units;
  }
  return (immune_system.empty() ? std::make_pair(false, total_units)
                                : std::make_pair(true, total_units));
}

int Battle::find_booster() {
  int low = 0;
  int high = 100000; // Arbitrary high value
  int result = -1;

  while (low <= high) {
    int mid = (low + high) / 2;
    Battle test_battle = *this;
    test_battle.boost(mid);
    auto outcome = test_battle.fight();
    if (outcome.first) {
      result = mid;
      high = mid - 1;
    } else {
      low = mid + 1;
    }
  }

  return result;
}

std::vector<int> Battle::target_selection() const {
  int n_immune = immune_system.size();
  int n_infect = infection.size();
  int total = n_immune + n_infect;

  std::vector<int> targets(total, -1);
  std::set<int> chosen;

  std::vector<const Army*> all;
  all.reserve(total);
  for (const auto& a : immune_system)
    all.push_back(&a);
  for (const auto& a : infection)
    all.push_back(&a);

  std::vector<int> order(total);
  for (int i = 0; i < total; ++i)
    order[i] = i;

  std::sort(order.begin(), order.end(), [&](int a, int b) {
    if (all[a]->effective_power() != all[b]->effective_power())
      return all[a]->effective_power() > all[b]->effective_power();
    return all[a]->initiative > all[b]->initiative;
  });

  for (int idx : order) {
    const Army& attacker = *all[idx];
    bool is_immune_system = (idx < n_immune);

    const auto& defenders = is_immune_system ? infection : immune_system;

    int best_target = -1;
    int max_damage = 0;
    int max_power = 0;
    int max_initiative = 0;

    for (size_t j = 0; j < defenders.size(); ++j) {
      int global_idx = is_immune_system ? n_immune + j : j;
      if (chosen.count(global_idx))
        continue;

      int damage = attacker.damage_to(defenders[j]);
      if (damage == 0)
        continue;

      int power = defenders[j].effective_power();
      int initiative = defenders[j].initiative;

      if (damage > max_damage || (damage == max_damage && power > max_power) ||
          (damage == max_damage && power == max_power && initiative > max_initiative)) {
        best_target = j;
        max_damage = damage;
        max_power = power;
        max_initiative = initiative;
      }
    }

    if (best_target != -1) {
      targets[idx] = best_target;
      chosen.insert(is_immune_system ? n_immune + best_target : best_target);
    }
  }

  return targets;
}

bool Battle::attack(const std::vector<int>& targets) {
  int n_immune = immune_system.size();
  int n_infect = infection.size();
  int total = n_immune + n_infect;

  std::vector<int> order(total);
  for (int i = 0; i < total; ++i)
    order[i] = i;

  auto get_army = [&](int idx) -> Army& {
    return (idx < n_immune) ? immune_system[idx] : infection[idx - n_immune];
  };

  std::sort(order.begin(), order.end(), [&](int a, int b) {
    return get_army(a).initiative > get_army(b).initiative;
  });
  int total_killed = 0;
  for (int idx : order) {
    Army& attacker = get_army(idx);
    if (attacker.units <= 0)
      continue;

    int target_local = targets[idx];
    if (target_local == -1)
      continue;

    bool is_immune_system = (idx < n_immune);
    int target_global = is_immune_system ? n_immune + target_local : target_local;
    if (target_global >= total)
      continue;

    Army& defender = get_army(target_global);
    if (defender.units <= 0)
      continue;

    int damage = attacker.damage_to(defender);
    int killed = damage / defender.hit_points;
    total_killed += std::min(defender.units, killed);
    defender.units -= std::min(defender.units, killed);
  }

  immune_system.erase(
      std::remove_if(
          immune_system.begin(), immune_system.end(), [](const Army& a) { return a.units <= 0; }),
      immune_system.end());

  infection.erase(
      std::remove_if(
          infection.begin(), infection.end(), [](const Army& a) { return a.units <= 0; }),
      infection.end());
  return total_killed > 0;
}
#ifndef STANDALONE
Battle get_battle(List immune_system_r, List infection_r) {
  std::vector<Army> immune_system;
  auto parse_to_set = [](const std::string& input) {
    std::set<std::string> result;
    if (!input.empty()) {
      std::stringstream ss(input);
      std::string item;
      while (std::getline(ss, item, ',')) {
        if (!item.empty())
          result.insert(item);
      }
    }
    return result;
  };
  for (const auto& group : immune_system_r) {
    List g = as<List>(group);
    Army army;
    army.units = as<int>(g["units"]);
    army.hit_points = as<int>(g["hp"]);
    army.attack_damage = as<int>(g["dmg"]);
    army.attack_type = as<std::string>(g["attack"]);
    army.initiative = as<int>(g["initiative"]);
    army.weaknesses = parse_to_set(as<std::string>(g["weak"]));
    army.immunities = parse_to_set(as<std::string>(g["immune"]));
    immune_system.push_back(army);
  }

  std::vector<Army> infection;
  for (const auto& group : infection_r) {
    List g = as<List>(group);
    Army army;
    army.units = as<int>(g["units"]);
    army.hit_points = as<int>(g["hp"]);
    army.attack_damage = as<int>(g["dmg"]);
    army.attack_type = as<std::string>(g["attack"]);
    army.initiative = as<int>(g["initiative"]);
    army.weaknesses = parse_to_set(as<std::string>(g["weak"]));
    army.immunities = parse_to_set(as<std::string>(g["immune"]));
    infection.push_back(army);
  }
  Battle battle(immune_system, infection);
  return battle;
}

// [[Rcpp::export]]
int count_survivors(List immune_system_r, List infection_r) {
  Battle battle = get_battle(immune_system_r, infection_r);
  auto result = battle.fight();
  return result.second;
}

// [[Rcpp::export]]
int find_minimum_booster(List immune_system_r, List infection_r) {
  Battle battle = get_battle(immune_system_r, infection_r);
  int booster = battle.find_booster();
  battle.boost(booster);
  auto result = battle.fight();
  return result.second;
}

#else
int main() {
  std::vector<Army> immune_system = {
      {17, 5390, 4507, "fire", 2, {"radiation", "bludgeoning"}, {}},
      {989, 1274, 25, "slashing", 3, {"bludgeoning", "slashing"}, {"fire"}}};
  std::vector<Army> infection = {{801, 4706, 116, "bludgeoning", 1, {"radiation"}, {}},
                                 {4485, 2961, 12, "slashing", 4, {"fire", "cold"}, {"radiation"}}};

  Battle battle(immune_system, infection);
  auto result = battle.fight();
  std::cout << (result.first ? "Immune system" : "Infection") << " won with " << result.second
            << " remaining units" << std::endl;
  return 0;
}
#endif
