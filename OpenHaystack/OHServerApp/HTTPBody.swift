//
//  OpenHaystack – Tracking personal Bluetooth devices via Apple's Find My network
//
//  Copyright © 2021 Secure Mobile Networking Lab (SEEMOO)
//  Copyright © 2021 The Open Wireless Link Project
//
//  SPDX-License-Identifier: AGPL-3.0-only
//

import Foundation
import Vapor

struct GetReportsBody: Codable {
    let ids: [String]
}

struct DecryptReportsRequestBody: Codable, Content {
    let ids: [IdWithKey]
    
    struct IdWithKey: Codable, Content {
        /// Base 64 encoded id (hash of the public key)
        let id: String
        /// Base64Encoded Private Key
        let privateKey: String
        
        var privateKeyData: Data? {
            Data(base64Encoded: privateKey)
        }
    }
}


struct DecryptedReportResponse: Codable, Content {
    let encryptedReport: FindMyReport
    let decryptedReport: FindMyLocationReport
}

extension FindMyLocationReport: Content {}
