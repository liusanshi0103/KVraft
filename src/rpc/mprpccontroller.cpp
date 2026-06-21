#include "mprpccontroller.h"

MprpcController::MprpcController()
    : failed_(false), error_text_("") {}

void MprpcController::Reset() {
  failed_ = false;
  error_text_.clear();
}

bool MprpcController::Failed() const {
  return failed_;
}

std::string MprpcController::ErrorText() const {
  return error_text_;
}

void MprpcController::SetFailed(const std::string& reason) {
  failed_ = true;
  error_text_ = reason;
}

void MprpcController::StartCancel() {}

bool MprpcController::IsCanceled() const {
  return false;
}

void MprpcController::NotifyOnCancel(google::protobuf::Closure* callback) {}