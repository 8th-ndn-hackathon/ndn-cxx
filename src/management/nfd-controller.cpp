/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/**
 * Copyright (C) 2013 Regents of the University of California.
 * See COPYING for copyright and distribution information.
 */

#include "common.hpp"
#include "../face.hpp"

#include "nfd-controller.hpp"
#include "nfd-fib-management-options.hpp"
#include "nfd-face-management-options.hpp"
#include "nfd-control-response.hpp"

namespace ndn {
namespace nfd {

Controller::Controller(Face& face)
  : m_face(face)
  , m_faceId(0)
{
}

void
Controller::selfRegisterPrefix(const Name& prefixToRegister,
                               const SuccessCallback& onSuccess,
                               const FailCallback&    onFail)
{
  startFibCommand("add-nexthop",
                  FibManagementOptions()
                  .setName(prefixToRegister)
                  .setFaceId(0) // self-registration
                  .setCost(0),
                  bind(&Controller::recordSelfRegisteredFaceId, this, _1, onSuccess),
                  onFail);
}

void
Controller::selfDeregisterPrefix(const Name& prefixToRegister,
                                 const SuccessCallback& onSuccess,
                                 const FailCallback&    onFail)
{
  if (m_faceId == 0)
    {
      if (static_cast<bool>(onFail))
        onFail("Face ID is not set, should have been set after a successful prefix registration command");
      return;
    }

  startFibCommand("remove-nexthop",
                  FibManagementOptions()
                  .setName(prefixToRegister)
                  .setFaceId(m_faceId),
                  bind(onSuccess), onFail);
}

void
Controller::startFibCommand(const std::string& command,
                            const FibManagementOptions& options,
                            const FibCommandSucceedCallback& onSuccess,
                            const FailCallback& onFail)
{
  Name fibCommandInterestName("/localhost/nfd/fib");
  fibCommandInterestName
    .append(command)
    .append(options.wireEncode());

  Interest fibCommandInterest(fibCommandInterestName);
  // m_keyChain.sign(fibCommandInterest);

  m_face.expressInterest(fibCommandInterest,
                         bind(&Controller::processFibCommandResponse, this, _2,
                              onSuccess, onFail),
                         bind(onFail, "Command Interest timed out"));
}

void
Controller::recordSelfRegisteredFaceId(const FibManagementOptions& entry,
                                       const SuccessCallback& onSuccess)
{
  m_faceId = entry.getFaceId();
  onSuccess();
}

void
Controller::processFibCommandResponse(Data& data,
                                      const FibCommandSucceedCallback& onSuccess,
                                      const FailCallback& onFail)
{
  try
    {
      ControlResponse response(data.getContent().blockFromValue());
      if (response.getCode() != 200)
        return onFail(response.getText());

      FibManagementOptions options(response.getBody());
      return onSuccess(options);
    }
  catch(ndn::Tlv::Error& e)
    {
      if (static_cast<bool>(onFail))
        return onFail(e.what());
    }
}

void
Controller::startFaceCommand(const std::string& command,
                             const FaceManagementOptions& options,
                             const FaceCommandSucceedCallback& onSuccess,
                             const FailCallback& onFail)
{
  Name faceCommandInterestName("/localhost/nfd/faces");
  faceCommandInterestName
    .append(command)
    .append(options.wireEncode());

  Interest faceCommandInterest(faceCommandInterestName);
  // m_keyChain.sign(fibCommandInterest);

  m_face.expressInterest(faceCommandInterest,
                         bind(&Controller::processFaceCommandResponse, this, _2,
                              onSuccess, onFail),
                         bind(onFail, "Command Interest timed out"));
}

void
Controller::processFaceCommandResponse(Data& data,
                                       const FaceCommandSucceedCallback& onSuccess,
                                       const FailCallback& onFail)
{
  try
    {
      ControlResponse response(data.getContent().blockFromValue());
      if (response.getCode() != 200)
        return onFail(response.getText());

      FaceManagementOptions options(response.getBody());
      return onSuccess(options);
    }
  catch(ndn::Tlv::Error& e)
    {
      if (static_cast<bool>(onFail))
        return onFail(e.what());
    }
}
} // namespace nfd
} // namespace ndn
