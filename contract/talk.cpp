#include <eosio/eosio.hpp>

// Message table
struct [[eosio::table("message"), eosio::contract("talk")]] message {
    uint64_t    id       = {}; // Non-0
    uint64_t    reply_to = {}; // Non-0 if this is a reply
    eosio::name user     = {};
    std::string content  = {};

    uint64_t primary_key() const { return id; }
    uint64_t get_reply_to() const { return reply_to; }
};

// Like table
struct [[eosio::table("likes"), eosio::contract("talk")]] likes {
    uint64_t id = {}; // Non-0
    uint64_t like_id = {}; // Non-0 if this is a reply
    eosio::name user = {};

    uint64_t primary_key() const { return id; }
    uint64_t get_like_id() const { return like_id; }
    eosio::name get_user() const { return user;}
};

using message_table = eosio::multi_index<
    "message"_n, message, eosio::indexed_by<"by.reply.to"_n, eosio::const_mem_fun<message, uint64_t, &message::get_reply_to>>>;
using likes_table = eosio::multi_index<
    "likes"_n, likes, eosio::indexed_by<"by.like.id"_n, eosio::const_mem_fun<likes, uint64_t, &likes::get_like_id>>>;

// The contract
class talk : eosio::contract {
  public:
    // Use contract's constructor
    using contract::contract;

    // Post a message
    [[eosio::action]] void post(uint64_t id, uint64_t reply_to, eosio::name user, const std::string& content) {
        message_table table{get_self(), 0};

        // Check user
        require_auth(user);

        // Check reply_to exists
        if (reply_to)
            table.get(reply_to);

        // Create an ID if user didn't specify one
        eosio::check(id < 1'000'000'000ull, "user-specified id is too big");
        if (!id)
            id = std::max(table.available_primary_key(), 1'000'000'000ull);

        // Record the message
        table.emplace(get_self(), [&](auto& message) {
            message.id       = id;
            message.reply_to = reply_to;
            message.user     = user;
            message.content  = content;
        });
    }

    // like a message
    [[eosio::action]] void like(uint64_t id, uint64_t like_id, eosio::name user) {
        likes_table table{get_self(), 0};

        // Check user
        require_auth(user);
        message_table message_table{get_self(), 0};

        // Check like_id exists
        if (like_id)
            message_table.get(like_id,"not foud");

        for(auto iter = table.begin();iter != table.end();++iter){
            if (iter->user == user && iter->like_id == like_id) {
                return;
            }
        }

        // Create an ID if user didn't specify one
        eosio::check(id < 1'000'000'000ull, "user-specified id is too big");
        if (!id)
            id = std::max(table.available_primary_key(), 1'000'000'000ull);

        // Record the like
        table.emplace(get_self(), [&](auto& like) {
            like.id       = id;
            like.like_id  = like_id;
            like.user     = user;
        });
    }

    // like a message
    [[eosio::action]] void unlike(uint64_t like_id, eosio::name user) {
        likes_table table{get_self(), 0};

        // Check user
        require_auth(user);
        message_table message_table{get_self(), 0};

        // Check like_id exists
        if (like_id)
            message_table.get(like_id,"not foud");
        
        for(auto iter = table.begin();iter != table.end();++iter){
            if (iter->user == user && iter->like_id == like_id) {
                table.erase(iter);
                return;
            }
        }
    }
};
