/*
 * Copyright (C) 2018-2024 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#if ENABLE(WEB_AUTHN)

#include "AuthenticatorManager.h"
#include <WebCore/MockWebAuthenticationConfiguration.h>

namespace WebKit {

class MockAuthenticatorManager final : public AuthenticatorManager {
    WTF_MAKE_TZONE_ALLOCATED(MockAuthenticatorManager);
public:
    static Ref<MockAuthenticatorManager> create(WebCore::MockWebAuthenticationConfiguration&&);

    bool isMock() const final { return true; }
    void setTestConfiguration(WebCore::MockWebAuthenticationConfiguration&& configuration) { m_testConfiguration = WTFMove(configuration); }

private:
    explicit MockAuthenticatorManager(WebCore::MockWebAuthenticationConfiguration&&);

    Ref<AuthenticatorTransportService> createService(WebCore::AuthenticatorTransport, AuthenticatorTransportServiceObserver&) const final;
    void respondReceivedInternal(Respond&&) final;
    void filterTransports(TransportSet&) const;
    void runPresenterInternal(const TransportSet&) final { }

    WebCore::MockWebAuthenticationConfiguration m_testConfiguration;
};

} // namespace WebKit

SPECIALIZE_TYPE_TRAITS_BEGIN(WebKit::MockAuthenticatorManager)
static bool isType(const WebKit::AuthenticatorManager& manager) { return manager.isMock(); }
SPECIALIZE_TYPE_TRAITS_END()

#endif // ENABLE(WEB_AUTHN)
