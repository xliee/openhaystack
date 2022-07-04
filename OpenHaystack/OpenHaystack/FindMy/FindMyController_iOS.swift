//
//  OpenHaystack – Tracking personal Bluetooth devices via Apple's Find My network
//
//  Copyright © 2021 Secure Mobile Networking Lab (SEEMOO)
//  Copyright © 2021 The Open Wireless Link Project
//
//  SPDX-License-Identifier: AGPL-3.0-only
//

import Foundation

extension FindMyController {
    func fetchReports(with searchPartyToken: Data, completion: @escaping (Error?) -> Void) {
        guard let url = self.serverURL else {
            completion(FindMyErrors.noServerURL)
            return
        }

        var request = URLRequest(url: url.appendingPathComponent("getLocationReports"))
        request.httpMethod = "POST"
        request.addValue("application/json", forHTTPHeaderField: "Content-Type")
        let keyHashes = devices.flatMap({ $0.keys }).map { $0.hashedKey.base64EncodedString() }
        request.httpBody = try! JSONEncoder().encode(GetReportsBody(ids: keyHashes))

        let task = URLSession(configuration: .default).dataTask(with: request) { data, response, error in
            guard let httpResp = response as? HTTPURLResponse,
                let respData = data
            else {
                completion(FindMyErrors.serverError(message: "Wrong response"))
                return
            }

            if httpResp.statusCode >= 300,
                let message = String(data: respData, encoding: .utf8)
            {
                completion(FindMyErrors.serverError(message: message))
                return
            }

            guard let locationReports = try? JSONDecoder().decode(FindMyReportResults.self, from: respData) else {
                completion(FindMyErrors.serverError(message: "Could not decode"))
                return
            }

            //Add each report to the correct device
            for report in locationReports.results {
                guard let device = self.devices.first(where: { $0.keys.contains(where: { $0.hashedKey.base64EncodedString() == report.id }) }) else { continue }
                if device.reports == nil {
                    device.reports = [FindMyReport]()
                }
                device.reports?.append(report)
            }

            DispatchQueue.global(qos: .userInitiated).async {
                //Decrypt the reports
                self.decryptReports {
                    DispatchQueue.main.async {
                        completion(nil)
                    }
                }
            }
        }
        task.resume()
    }

}
