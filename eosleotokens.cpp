#include <string>
#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>

#include <eosiolib/transaction.hpp>
#include <eosiolib/crypto.h>

using eosio::indexed_by;
using eosio::const_mem_fun;
using eosio::asset;
using eosio::permission_level;
using eosio::action;
using eosio::print;
using eosio::name;
using std::string;
using eosio::name;
using eosio::string_to_name;



class eosleotokens : public eosio::contract {

  const std::string TEAM_ACCOUNT = "eosioleoteam"; // team account
  const uint64_t EXCHANGE_RATIO = 500; // exchange ratio
  const uint64_t COIN_UNIT = 10000; // coin unit
  const uint64_t INITIAL_QUANTITY = 100 * COIN_UNIT; // initial quantity
  const uint64_t REWARD_QUANTITY = 100 * COIN_UNIT; // reward quantity

  public:
    eosleotokens(account_name self):eosio::contract(self),users(_self, _self){}
    
    void transfer( account_name from, account_name to, asset quantity, std::string memo ) {

        require_auth( from );

        if(quantity.is_valid() && quantity.symbol == S(4, EOS) && from != _self && to == _self)
        {
            if(quantity.amount==1)
            {
                auto useritr = users.find( from );
                eosio_assert( useritr == users.end(), "Account has claimed.");

                users.emplace( _self, [&]( auto& s ) {
                        s.n = from;
                        s.e = 0;
                        s.k = INITIAL_QUANTITY;
                });


                asset balance(INITIAL_QUANTITY, S(4, ELE));
                action(
                    permission_level{ N(eosioleoteam), N(active) },
                    N(eosioleoteam), N(issue),
                    std::make_tuple(from, balance, std::string("Thanks for support：https://eosleo.io"))
                ).send();

            }else if(quantity.amount >= 1000){
                eosio_assert( quantity.amount <= 1000*10000, "Once should less than 1000 EOS" );
                eosio_assert( memo.size() <= 256, "Memo should less than 256 bit" );

                uint64_t eos = quantity.amount;
                uint64_t key = eos * EXCHANGE_RATIO;

                auto parent = string_to_name(memo.c_str());
                auto parentitr = users.find( parent );
                if(memo.size()<=0 || memo.size()>12 || parent==_self || from==parent || parentitr==users.end())
                {
                    //legal: do nothing
                } else{
                    // reward father
		    users.modify( parentitr,0, [&]( auto& s ) {
                        s.k += REWARD_QUANTITY;
                    });

                    asset balance(REWARD_QUANTITY, S(4, ELE));
                    action(
                        permission_level{ N(eosioleoteam), N(active) },
                        N(eosioleoteam), N(issue),
                        std::make_tuple(parent, balance, std::string("Thanks for support：https://eosleo.io"))
                    ).send();
                }

                // record
                auto useritr = users.find( from );
                if( useritr == users.end() ) {
                    users.emplace( _self, [&]( auto& s ) {
                        s.n = from;
                        s.e = eos;
                        s.k = key;
                    });
                } else{
                    users.modify( useritr,0, [&]( auto& s ) {
			s.e += eos;
                        s.k += key;
                    });            
                }

                asset balance(key, S(4, ELE));
                action(
                    permission_level{ N(eosioleoteam), N(active) },
                    N(eosioleoteam), N(issue),
                    std::make_tuple(from, balance, std::string("Thanks for support：https://eosleo.io"))
                ).send();

            }
        }

    }

  private:

    // @abi table users i64
    struct user {
      account_name n;
      uint64_t e;
      uint64_t k;

      uint64_t primary_key() const { return n; }
      uint64_t get_key() const { return k; }
      EOSLIB_SERIALIZE(user, (n)(e)(k))
    };
    typedef eosio::multi_index<N(users), user,
    indexed_by<N(k), const_mem_fun<user, uint64_t, &user::get_key>>
    > user_list;
    user_list users;
};

 #define EOSIO_ABI_EX( TYPE, MEMBERS ) \
 extern "C" { \
    void apply( uint64_t receiver, uint64_t code, uint64_t action ) { \
       if( action == N(onerror)) { \
          eosio_assert(code == N(eosio), "onerror action's are only valid from the \"eosio\" system account"); \
       } \
       auto self = receiver; \
       if(code == N(eosio.token) && action == N(transfer)) { \
          TYPE thiscontract( self ); \
          switch( action ) { \
             EOSIO_API( TYPE, MEMBERS ) \
          } \
       } \
    } \
 }

EOSIO_ABI_EX(eosleotokens, (transfer))
