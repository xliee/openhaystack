//
//  OpenHaystack – Tracking personal Bluetooth devices via Apple's Find My network
//
//  Copyright © 2021 Secure Mobile Networking Lab (SEEMOO)
//  Copyright © 2021 The Open Wireless Link Project
//
//  SPDX-License-Identifier: AGPL-3.0-only
//

import CoreLocation
import Foundation

struct FindMyDevice: Codable, Hashable {

    let deviceId: String
    var keys = [FindMyKey]()

    var catalinaBigSurKeyFiles: [Data]?

    /// KeyHash: Report results.
    var reports: [FindMyReport]?

    var decryptedReports: [FindMyLocationReport]?

    func hash(into hasher: inout Hasher) {
        hasher.combine(deviceId)
    }

    static func == (lhs: FindMyDevice, rhs: FindMyDevice) -> Bool {
        lhs.deviceId == rhs.deviceId
    }
}

struct FindMyKey: Codable {
    internal init(advertisedKey: Data, hashedKey: Data, privateKey: Data, startTime: Date?, duration: Double?, pu: Data?, yCoordinate: Data?, fullKey: Data?) {
        self.advertisedKey = advertisedKey
        self.hashedKey = hashedKey
        // The private key should only be 28 bytes long. If a 85 bytes full private public key is entered we truncate it here
        if privateKey.count == 85 {
            self.privateKey = privateKey.subdata(in: 57..<privateKey.endIndex)
        } else {
            self.privateKey = privateKey
        }

        self.startTime = startTime
        self.duration = duration
        self.pu = pu
        self.yCoordinate = yCoordinate
        self.fullKey = fullKey
    }

    init(from decoder: Decoder) throws {
        let container = try decoder.container(keyedBy: CodingKeys.self)
        self.advertisedKey = try container.decode(Data.self, forKey: .advertisedKey)
        self.hashedKey = try container.decode(Data.self, forKey: .hashedKey)
        let privateKey = try container.decode(Data.self, forKey: .privateKey)
        if privateKey.count == 85 {
            self.privateKey = privateKey.subdata(in: 57..<privateKey.endIndex)
        } else {
            self.privateKey = privateKey
        }

        self.startTime = try? container.decode(Date.self, forKey: .startTime)
        self.duration = try? container.decode(Double.self, forKey: .duration)
        self.pu = try? container.decode(Data.self, forKey: .pu)
        self.yCoordinate = try? container.decode(Data.self, forKey: .yCoordinate)
        self.fullKey = try? container.decode(Data.self, forKey: .fullKey)
    }

    /// The advertising key.
    let advertisedKey: Data
    /// Hashed advertisement key using SHA256.
    let hashedKey: Data
    /// The private key from which the advertisement keys can be derived.
    let privateKey: Data
    /// When this key was used to send out BLE advertisements.
    let startTime: Date?
    /// Duration from start time how long the key has been used to send out BLE advertisements.
    let duration: Double?
    /// ?
    let pu: Data?

    /// As exported from Big Sur.
    let yCoordinate: Data?
    /// As exported from Big Sur.
    let fullKey: Data?
}

struct FindMyReportResults: Codable {
    let results: [FindMyReport]
}

struct FindMyReport: Codable {
    let datePublished: Date
    let payload: Data
    let id: String
    let statusCode: Int

    let confidence: UInt8
    let timestamp: Date

    enum CodingKeys: CodingKey {
        case datePublished
        case payload
        case id
        case statusCode
    }

    init(from decoder: Decoder) throws {
        let values = try decoder.container(keyedBy: CodingKeys.self)
        let dateTimestamp = try values.decode(Double.self, forKey: .datePublished)
        // Convert from milis to time interval
        let dP = Date(timeIntervalSince1970: dateTimestamp / 1000)
        let df = DateFormatter()
        df.dateFormat = "YYYY-MM-dd"

        if dP < df.date(from: "2020-01-01")! {
            self.datePublished = Date(timeIntervalSince1970: dateTimestamp)
        } else {
            self.datePublished = dP
        }

        self.statusCode = try values.decode(Int.self, forKey: .statusCode)
        let payloadBase64 = try values.decode(String.self, forKey: .payload)

        guard let payload = Data(base64Encoded: payloadBase64) else {
            throw DecodingError.dataCorruptedError(forKey: CodingKeys.payload, in: values, debugDescription: "")
        }
        self.payload = payload

        var timestampData = payload.subdata(in: 0..<4)
        let timestamp: Int32 = withUnsafeBytes(of: &timestampData) { (pointer) -> Int32 in
            // Convert the endianness
            pointer.load(as: Int32.self).bigEndian
        }

        // It's a cocoa time stamp (counting from 2001)
        self.timestamp = Date(timeIntervalSinceReferenceDate: TimeInterval(timestamp))
        self.confidence = payload[4]

        self.id = try values.decode(String.self, forKey: .id)
    }

    func encode(to encoder: Encoder) throws {
        var container = encoder.container(keyedBy: CodingKeys.self)
        try container.encode(self.datePublished.timeIntervalSince1970 * 1000, forKey: .datePublished)
        try container.encode(self.payload.base64EncodedString(), forKey: .payload)
        try container.encode(self.id, forKey: .id)
        try container.encode(self.statusCode, forKey: .statusCode)
    }
}

struct FindMyLocationReport: Codable {
    let latitude: Double
    let longitude: Double
    let accuracy: UInt8
    let datePublished: Date
    let timestamp: Date?
    let confidence: UInt8?
    let status: UInt8

    var location: CLLocation {
        return CLLocation(latitude: latitude, longitude: longitude)
    }

    init(lat: Double, lng: Double, acc: UInt8, dP: Date, t: Date, c: UInt8, s: UInt8 = 0) {
        self.latitude = lat
        self.longitude = lng
        self.accuracy = acc
        self.datePublished = dP
        self.timestamp = t
        self.confidence = c
        self.status = s
    }

    init(from decoder: Decoder) throws {
        let values = try decoder.container(keyedBy: CodingKeys.self)

        self.latitude = try values.decode(Double.self, forKey: .latitude)
        self.longitude = try values.decode(Double.self, forKey: .longitude)
        self.status = (try? values.decode(UInt8.self, forKey: .status)) ?? 0

        do {
            let uAcc = try values.decode(UInt8.self, forKey: .accuracy)
            self.accuracy = uAcc
        } catch {
            let iAcc = try values.decode(Int8.self, forKey: .accuracy)
            self.accuracy = UInt8(bitPattern: iAcc)
        }

        self.datePublished = try values.decode(Date.self, forKey: .datePublished)
        self.timestamp = try? values.decode(Date.self, forKey: .timestamp)
        self.confidence = try? values.decode(UInt8.self, forKey: .confidence)
    }

}

enum FindMyError: Error {
    case decryptionError(description: String)
}
