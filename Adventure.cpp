#include "Adventure.h"
#include <iostream>

// ===================== GIVEN HELPERS (do not change) =====================

// Reads the next line that is not blank.
// Spaces at the start of the line are removed.
// Returns false when the file has no more lines.
bool Adventure::nextLine(std::ifstream& in, std::string& line) {
    std::string raw;
    while (std::getline(in, raw)) {
        int start = 0;
        while (start < (int)raw.size() && (raw[start] == ' ' || raw[start] == '\t')) {
            start++;
        }
        line = raw.substr(start);
        // Files saved on Windows can end each line with an extra '\r'.
        if (!line.empty() && line[line.size() - 1] == '\r') {
            line = line.substr(0, line.size() - 1);
        }
        if (!line.empty()) {
            return true;
        }
    }
    return false;
}

// Reads the next line and checks that it starts with keyword.
// Example: for the line "HP -2" and keyword "HP", value becomes "-2".
// Prints an error and returns false if the line is missing or wrong.
bool Adventure::readField(std::ifstream& in, const std::string& keyword, std::string& value) {
    std::string line;
    if (!nextLine(in, line)) {
        std::cout << "Error: expected a " << keyword << " line but the file ended." << std::endl;
        return false;
    }
    std::string prefix = keyword + " ";
    if (line.substr(0, prefix.size()) != prefix) {
        std::cout << "Error: expected a " << keyword << " line but found: " << line << std::endl;
        return false;
    }
    value = line.substr(prefix.size());
    return true;
}

// Adds one choice number to a path.
// extendPath("", 2) gives "2". extendPath("2-1", 3) gives "2-1-3".
std::string Adventure::extendPath(const std::string& path, int choiceNumber) const {
    if (path.empty()) {
        return std::to_string(choiceNumber);
    }
    return path + "-" + std::to_string(choiceNumber);
}

// ===================== LOADING =====================

bool Adventure::load(const std::string& fileName) {
    std::ifstream in(fileName);
    if (!in.is_open()) {
        std::cout << "Error: could not open file " << fileName << std::endl;
        return false;
    }

    std::string value;
    if (!readField(in, "START_HP", value)) {
        return false;
    }
    startHp_ = std::stoi(value);

    Scene<Moment>* root = readScene(in, "START");
    if (root == nullptr) {
        return false;
    }

    tree_.setRoot(root);
    return true;
}

// TODO 11: read ONE scene and everything below it. Return it.
//   - Read SCENE, HP, LOOT, ITEM, and CHOICES with readField, in that order.
//   - If CHOICES is below 0 or above Scene<Moment>::MAX_CHOICES, print exactly:
//       Error: scene "<scene text>" has <n> choices. The limit is 3.
//     and return nullptr.
//   - Each choice is a LABEL line followed by a whole scene.
//   - If anything goes wrong at any depth, return nullptr
//     and make sure nothing you already built is leaked.
Scene<Moment>* Adventure::readScene(std::ifstream& in, const std::string& label) {
    std::string text, hpStr, lootStr, item, choicesStr;
    if (!readField(in, "SCENE", text))  return nullptr;
    if (!readField(in, "HP", hpStr)) return nullptr;
    if (!readField(in, "LOOT", lootStr)) return nullptr;
    if (!readField(in, "ITEM", item)) return nullptr;
    if (!readField(in, "CHOICE", choicesStr)) return nullptr;

    int numChoices = std::stoi(choicesStr);
    if (numChoices < 0 || numChoices > Scene<Moment>::MAX_CHOICES) {
        std::cout << "Error: scene \"" << text << "\" has " << numChoices
                  << " choices. The limit is 3." << std::endl;
        return nullptr;                 // nothing allocated yet, so nothing leaks
    }

    Moment* moment = new Moment(text, label, std::stoi(hpStr), std::stoi(lootStr), item);
    Scene<Moment>* scene = new Scene<Moment>(moment, numChoices);


    for (int i = 0; i < numChoices; i++) {
        std::string childLabel;
        if (!readField(in, "LABEL", childLabel)) {
            delete scene;
            return nullptr;
        }
        Scene<Moment>* child = readScene(in, childLabel);   // the recursion
        if (child == nullptr) {
            delete scene;
            return nullptr;
        }
        scene->setChoice(i, child);
    }
    return scene;

}

// ===================== PLAYING =====================

int Adventure::askChoice(int numChoices) {
    while (true) {
        std::cout << "Your choice: ";
        std::string line;
        if (!std::getline(std::cin, line)) {
            return 0;
        }
        if (line.size() == 1) {
            int n = line[0] - '0';
            if (n >= 1 && n <= numChoices) {
                return n;
            }
        }
        std::cout << "Please type a number from 1 to " << numChoices << "." << std::endl;
    }


}

// TODO 13: play the story once from the start.
// Every time the player ENTERS a scene, in this order:
//   1. Add its HP change to hp and its loot to loot.
//   2. Print a line of 40 dashes, then the scene text.
//   3. If the scene has an item:
//        if the bag already has it, print "You already have: <item>"
//        otherwise add it to the bag and print "You picked up: <item>"
//   4. Print "HP: <hp>  Loot: <loot>" (two spaces before Loot).
//   5. If hp is 0 or less, print "Your HP hit 0. GAME OVER." and stop.
//      If the scene is an ending, print "THE END." and stop.
//   6. Print each choice as "[1] <label>" and use askChoice.
//      If askChoice returns 0, leave play right away.
// When the game stops, print:
//   Final HP: <hp>  Final loot: <loot>
//   Inventory (<count> items):
// and then print the bag.
// The bag is a List<Item>. Start with HP = startHp_ and loot = 0.
void Adventure::play() {
    int hp = startHp_;
    int loot = 0;
    List<Item> bag;
    Scene<Moment>* scene = tree_.getRoot();

    while (scene != nullptr) {
        Moment* m = scene->getData();

        // 1. Enter the scene: apply HP and loot
        hp += m->getHpChange();
        loot += m->getLoot();

        // 2. Dashes, then the scene text
        std::cout << std::string(40, '-') << std::endl;
        std::cout << m->getText() << std::endl;

        // 3. Pick up the item, unless we already have it
        if (m->hasItem()) {
            Item found(m->getItem());
            if (bag.search(found)) {
                std::cout << "You already have: " << m->getItem() << std::endl;
            } else {
                bag.addFront(new Item(m->getItem()));
                std::cout << "You picked up: " << m->getItem() << std::endl;
            }
        }

        // 4. Status line (two spaces before Loot)
        std::cout << "HP: " << hp << "  Loot: " << loot << std::endl;

        // 5. Dead? Ending?
        if (hp <= 0) {
            std::cout << "Your HP hit 0. GAME OVER." << std::endl;
            break;
        }
        if (scene->isEnding()) {
            std::cout << "THE END." << std::endl;
            break;
        }

        // 6. Show the choices and ask
        for (int i = 0; i < scene->getNumChoices(); i++) {
            std::cout << "[" << (i + 1) << "] "
                      << scene->getChoice(i)->getData()->getLabel() << std::endl;
        }
        int pick = askChoice(scene->getNumChoices());
        if (pick == 0) {
            return;                         // input ended, so leave right away
        }
        scene = scene->getChoice(pick - 1); // door 1 is slot 0
    }

    std::cout << "Final HP: " << hp << "  Final loot: " << loot << std::endl;
    std::cout << "Inventory (" << bag.size() << " items):" << std::endl;
    bag.print();
}

// ===================== ANALYZING =====================


void Adventure::printStats() const {
    std::cout << "Scenes: " << tree_.countScenes() << std::endl;
    std::cout << "Endings: " << tree_.countEndings() << std::endl;
    std::cout << "Longest playthrough: " << tree_.height() << " choices" << std::endl;
}

void Adventure::printSpoilers() const {
    printEndings(tree_.getRoot(), "");
}

void Adventure::printEndings(Scene<Moment>* scene, const std::string& path) const {
    if (scene == nullptr) {
        return;
    }
    if (scene->isEnding()) {
        if (path.empty()) {
            std::cout << "Path (start): " << scene->getData()->getText() << std::endl;
        } else {
            std::cout << "Path " << path << ": " << scene->getData()->getText() << std::endl;
        }
        return;
    }
    for (int i = 0; i < scene->getNumChoices(); i++) {
        printEndings(scene->getChoice(i), extendPath(path, i + 1));
    }
}


// Start at the first scene with startHp_ and 0 loot.
// Enter scenes the same way play does (add HP and loot).
// Stop when hp is 0 or less, or you reach an ending.
// Otherwise always move to the choice with the MOST loot right now.
// On a tie, pick the lower choice number.
// Print:
//   Greedy advisor
//     Path: <path>
//     Loot: <loot>
//     Result: died               (or "Result: reached an ending")
void Adventure::greedyRun() const {
    Scene<Moment>* scene = tree_.getRoot();
    if (scene == nullptr) {
        return;
    }
    int hp = startHp_;
    int loot = 0;
    std::string path = "";


    hp += scene->getData()->getHpChange();
    loot += scene->getData()->getLoot();

    while (hp > 0 && !scene->isEnding()) {

        int bestIndex = 0;
        for (int i = 1; i < scene->getNumChoices(); i++) {
            if (scene->getChoice(i)->getData()->getLoot() >
                scene->getChoice(bestIndex)->getData()->getLoot()) {
                bestIndex = i;
                }
        }

        scene = scene->getChoice(bestIndex);
        path = extendPath(path, bestIndex + 1);
        hp += scene->getData()->getHpChange();
        loot += scene->getData()->getLoot();
    }

    std::cout << "Greedy advisor" << std::endl;
    std::cout << "  Path: " << path << std::endl;
    std::cout << "  Loot: " << loot << std::endl;
    if (hp <= 0) {
        std::cout << "  Result: died" << std::endl;
    } else {
        std::cout << "  Result: reached an ending" << std::endl;
    }
}


int Adventure::bestLoot(Scene<Moment>* scene, int hp, const std::string& path, std::string& bestPath) const {
    if (scene == nullptr) {
        return -1;
    }

    hp += scene->getData()->getHpChange();
    if (hp <= 0) {
        return -1;
    }
    int here = scene->getData()->getLoot();

    if (scene->isEnding()) {
        bestPath = path;
        return here;
    }

    int best = -1;
    for (int i = 0; i < scene->getNumChoices(); i++) {
        std::string childPath;
        int result = bestLoot(scene->getChoice(i), hp, extendPath(path, i + 1), childPath);
        if (result > best) {
            best = result;
            bestPath = childPath;
        }
    }

    if (best == -1) {
        return -1;
    }
    return here + best;
}


// Print:
//   Optimal advisor
//     Path: <path>
//     Loot: <loot>
//     Result: reached an ending
// If no path survives, print only:
//   Optimal advisor
//     Result: no path survives this story
void Adventure::optimalRun() const {
    std::string bestPath;
    int loot = bestLoot(tree_.getRoot(), startHp_, "", bestPath);

    std::cout << "Optimal advisor" << std::endl;
    if (loot == -1) {
        std::cout << "  Result: no path survives this story" << std::endl;
        return;
    }
    std::cout << "  Path: " << bestPath << std::endl;
    std::cout << "  Loot: " << loot << std::endl;
    std::cout << "  Result: reached an ending" << std::endl;
}
