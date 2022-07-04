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

struct ServerReportsFetcher {
    static func fetchReports(for ids: [String], with req: Request) -> EventLoopFuture<Response> {

        let promise = req.eventLoop.makePromise(of: Response.self)
        var finished = false

        let reportsFetcher = ReportsFetcher()

        DispatchQueue.global(qos: .background).asyncAfter(deadline: .now() + 20) {
            guard !finished else { return }
            promise.succeed(Response(status: .notFound, body: Response.Body(staticString: "Did not fetch anisette data in time")))
        }

        //Get the anisette data and token
        AnisetteDataManager.shared.requestAnisetteData { result in
            switch result {
            case .failure(let error):
                print(error)
                finished = true
                promise.succeed(Response(status: .notFound, body: Response.Body(staticString: "Anisette data not available")))

            case .success(let accountData):
                //Fetch the reports
                guard let token = accountData.searchPartyToken,
                    token.isEmpty == false
                else {
                    finished = true
                    promise.succeed(Response(status: .notFound, body: Response.Body(staticString: "Search party token not available")))
                    return
                }

                // 7 days
                let duration: Double = (24 * 60 * 60) * 21
                let startDate = Date() - duration
                reportsFetcher.query(
                    forHashes: ids,
                    start: startDate,
                    duration: duration,
                    searchPartyToken: token
                ) { responseData in
                    finished = true
                    promise.succeed(Response(status: .ok, body: Response.Body(data: responseData ?? Data())))
                }

            }
        }

        return promise.futureResult
    }
    
    static func decryptReports(findMyResults: FindMyReportResults, idsAndKeys: [DecryptReportsRequestBody.IdWithKey]) -> [DecryptedReportResponse] {
        
        var keyMap: [String: FindMyKey] = [:]
        idsAndKeys.forEach { idAndKey in
            keyMap[idAndKey.id] = FindMyKey(advertisedKey: Data(), hashedKey: Data(), privateKey: idAndKey.privateKeyData ?? Data(), startTime: nil, duration: nil, pu: nil, yCoordinate: nil, fullKey: nil)
        }
        
        let accessQueue = DispatchQueue(label: "threadSafeAccess", qos: .userInitiated, attributes: .concurrent, autoreleaseFrequency: .workItem, target: nil)
        var decryptedReports: [DecryptedReportResponse]!
        accessQueue.sync {
            decryptedReports = [DecryptedReportResponse](repeating:
                                                            DecryptedReportResponse(encryptedReport: findMyResults.results[0],
                                                                                    decryptedReport: FindMyLocationReport(lat: 0, lng: 0, acc: 0, dP: Date(), t: Date(), c: 0))
                                                            , count: findMyResults.results.count)
        }
        DispatchQueue.concurrentPerform(iterations: findMyResults.results.count) { (reportIdx) in
            let report = findMyResults.results[reportIdx]
            guard let key = keyMap[report.id] else { return }
            do {
                // Decrypt the report
                let locationReport = try DecryptReports.decrypt(report: report, with: key)
                accessQueue.async(flags: .barrier) {
                    let response = DecryptedReportResponse(encryptedReport: report, decryptedReport: locationReport)
                    decryptedReports[reportIdx] = response
                }
            } catch {
                return
            }
        }
        
        return decryptedReports
    }
}
