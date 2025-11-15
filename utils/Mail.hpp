#pragma once

#include <string>
#include <json.hpp>

using json = nlohmann::json;

class Mail {
public:

    Mail() {};

    Mail(const std::string& sender, const std::string& receiver,
            const std::string& subject, const std::string& message)
        : sender_(sender), receiver_(receiver),
            subject_(subject), message_(message) {}

    inline std::string serialize() const {
        return "Sender:" + sender_ + "\n" +
                "Receiver:" + receiver_ + "\n" +
                "Subject:" + subject_ + "\n" +
                "Message:" + message_ + "\n";
    }

    std::string& refSender() { return sender_; }
    std::string& refReceiver() { return receiver_; }
    std::string& refSubject() { return subject_; }
    std::string& refMessage() { return message_; }

    std::string getSender() const { return sender_; }
    std::string getReceiver() const { return receiver_; }
    std::string getSubject() const { return subject_; }
    std::string getMessage() const { return message_; }

private:
    std::string sender_;
    std::string receiver_;
    std::string subject_;
    std::string message_;
};

void to_json(json& j, const Mail& m);

void from_json(const json& j, Mail& m);