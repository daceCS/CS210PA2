#pragma once
#include "Scene.h"

// The whole story. It knows the first scene and nothing else.
//
// RULES (do not change):
//   - Keep every name and signature exactly as written.
//   - The private helpers must be recursive. No loops that walk down the tree.
//   - This class must work for ANY T. Never mention Moment in here.
template <typename T>
class StoryTree {
private:
    Scene<T>* root_;

    int countScenes(Scene<T>* scene) const {
        if (scene == nullptr) {
            return 0;
        }
        int total = 1;
        for (int i = 0; i < scene->getNumChoices(); i++) {
            total += countScenes(scene->getChoice(i));
        }
        return total;
    }

    int countEndings(Scene<T>* scene) const {
        if (scene == nullptr) {
            return 0;
        }
        if (scene->isEnding()) {
            return 1;
        }
        int total = 0;
        for (int i = 0; i<scene->getNumChoices(); i++) {
            total += countEndings(scene->getChoice(i));
        }
        return total;
    }

    int height(Scene<T>* scene) const {
        if (scene == nullptr) {
            return -1;
        }
        int best = -1;
        for (int i = 0; i<scene->getNumChoices(); i++) {
            int h = height(scene->getChoice(i));
            if (h > best) {
                best = h;
            }
        }
        return best+1;
    }

public:
    StoryTree() : root_(nullptr) {}

    ~StoryTree() {
        clear();
    }

    void setRoot(Scene<T>* root) {
        clear();
        root_ = root;
    }

    Scene<T>* getRoot() const {
        return root_;
    }

    bool isEmpty() const {
        return root_ == nullptr;
    }

    void clear() {
        delete root_;
        root_ = nullptr;
    }

    int countScenes() const {
        return countScenes(root_);
    }

    int countEndings() const {
        return countEndings(root_);
    }

    int height() const {
        return height(root_);
    }
};
