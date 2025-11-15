#include "Mail.hpp"

void to_json(json& j, const Mail& m) {
    j = json::object({{"sender", m.getSender()}, {"receiver", m.getReceiver()}, {"subject", m.getSubject()}, {"message", m.getMessage()}});
}

void from_json(const json& j, Mail& m) {
    j.at("sender").get_to(m.refSender());
    j.at("receiver").get_to(m.refReceiver());
    j.at("subject").get_to(m.refSubject());
    j.at("message").get_to(m.refMessage());
}