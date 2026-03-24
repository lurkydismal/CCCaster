use serde::{Deserialize, Serialize};
use std::collections::{BTreeMap, BTreeSet};

#[derive(Clone, Debug, Default, Serialize, Deserialize)]
#[serde(default)]
pub struct Profile {
    pub enabled_mods: BTreeSet<String>,
    pub load_order: Option<Vec<String>>,
    pub env: BTreeMap<String, String>,
    pub game_args: Vec<String>,
}

impl Profile {
    pub fn effective_load_order(&self) -> Vec<String> {
        let mut result = Vec::new();
        let mut seen = BTreeSet::new();

        // Apply load_order (filtered by enabled_mods)
        if let Some(l_order) = &self.load_order {
            for l_mod in l_order {
                if self.enabled_mods.contains(l_mod) {
                    result.push(l_mod.clone());
                    seen.insert(l_mod.clone());
                }
            }
        }

        // Append enabled mods not in load_order
        for l_mod in &self.enabled_mods {
            if !seen.contains(l_mod) {
                result.push(l_mod.clone());
            }
        }

        result
    }

    pub fn load_order_string(&self) -> String {
        self.effective_load_order().join(";")
    }

    #[expect(unused)]
    pub fn has_mod(&self, mod_name: &str) -> bool {
        self.enabled_mods.contains(mod_name)
    }

    pub fn set_env_overrides_into(&self, target: &mut BTreeMap<String, String>) {
        for (l_key, l_value) in &self.env {
            target.insert(l_key.clone(), l_value.clone());
        }
    }

    pub fn append_game_args_into(&self, target: &mut Vec<String>) {
        target.extend(self.game_args.iter().cloned());
    }
}
