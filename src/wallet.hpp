#pragma once
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>
#include "economy_manager.hpp"

using namespace godot;

class Wallet : public Node {
    GDCLASS(Wallet, Node)

public:
    Wallet() {}
    ~Wallet() override {}

    void add_currency(const String &currency_id, float amount);
    bool remove_currency(const String &currency_id, float amount);
    bool has_currency(const String &currency_id, float amount) const;
    bool transfer_currency_to(Wallet* other, const String &currency_id, float amount);

    void add_item(const String &item_id, float amount);
    bool remove_item(const String &item_id, float amount);
    bool has_item(const String &item_id, float amount) const;
    bool transfer_item_to(Wallet* other, const String &item_id, float amount);

    void _ready() override;

    void _on_economy_manager_ready(Object *mgr_object);

    void _on_node_added(Node*node);
    
protected:
    static void _bind_methods();
    
private:
    Dictionary balances_items;
    Dictionary balances_currencies;
    EconomyManager* economy_manager;
};
