// Kept in its own file: libultraship's controller deck headers pull in its libultra types, which clash with
// ultramodern's.

#include <memory>

#include <libultraship/controller/controldeck/ControlDeck.h>

#include "lus_menu.h"

namespace bm64lus {
    std::shared_ptr<Ship::ControlDeck> create_control_deck() {
        return std::make_shared<LUS::ControlDeck>();
    }
}
