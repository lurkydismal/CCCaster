#include "MouseManager.hpp"

MouseManager& MouseManager::get() {
    static MouseManager instance;
    return instance;
}
