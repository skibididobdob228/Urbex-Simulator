#include "core/common.h"

using namespace godot;

namespace urbex {

String item_display_name(const String &id) {
	if (id == "magnet") {
		return "Неодимовый магнит"_u;
	}
	if (id == "boltcutter") {
		return "Болторез"_u;
	}
	if (id == "key_attic") {
		return "Ключ от чердака"_u;
	}
	if (id == "key_booth") {
		return "Ключ от будки ЧОП"_u;
	}
	if (id == "key_shelter") {
		return "Ключ от убежища"_u;
	}
	if (id == "key_roof") {
		return "Ключ от люка"_u;
	}
	if (id == "key_intercom") {
		return "Ключ от домофона"_u;
	}
	if (id == "gasmask") {
		return "Противогаз ГП-5"_u;
	}
	if (id == "batteries") {
		return "Батарейки"_u;
	}
	if (id == "medcard") {
		return "Медкарта 1984 года"_u;
	}
	return id;
}

} // namespace urbex
