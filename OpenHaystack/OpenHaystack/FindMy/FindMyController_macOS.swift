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

        DispatchQueue.global(qos: .background).async {
            let fetchReportGroup = DispatchGroup()

            let fetcher = ReportsFetcher()

            var devices = self.devices
            for deviceIndex in 0..<devices.count {
                fetchReportGroup.enter()
                devices[deviceIndex].reports = []

                // Only use the newest keys for testing
                let keys = devices[deviceIndex].keys

                let keyHashes = keys.map({ $0.hashedKey.base64EncodedString() })

                // 21 days
                let duration: Double = (24 * 60 * 60) * 21
                let startDate = Date() - duration

                fetcher.query(forHashes: keyHashes, start: startDate, duration: duration, searchPartyToken: searchPartyToken) { jd in
                    guard let jsonData = jd else {
                        fetchReportGroup.leave()
                        return
                    }

                    do {
                        // Decode the report
                        let report = try JSONDecoder().decode(FindMyReportResults.self, from: jsonData)
                        devices[deviceIndex].reports = report.results

                    } catch {
                        print("Failed with error \(error)")
                        devices[deviceIndex].reports = []
                    }
                    fetchReportGroup.leave()
                }

            }

            // Completion Handler
            fetchReportGroup.notify(queue: .main) {
                print("Finished loading the reports. Now decrypt them")

                // Export the reports to the desktop
                var reports = [FindMyReport]()
                for device in devices {
                    for report in device.reports! {
                        reports.append(report)
                    }
                }

                #if EXPORT
                    if let encoded = try? JSONEncoder().encode(reports) {
                        let outputDirectory = FileManager.default.urls(for: .desktopDirectory, in: .userDomainMask).first!
                        try? encoded.write(to: outputDirectory.appendingPathComponent("reports.json"))
                    }
                #endif

                DispatchQueue.main.async {
                    self.devices = devices

                    self.decryptReports {
                        completion(nil)
                    }

                }
            }
        }

    }
}
