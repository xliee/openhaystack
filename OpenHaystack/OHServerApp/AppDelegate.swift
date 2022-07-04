//
//  OpenHaystack – Tracking personal Bluetooth devices via Apple's Find My network
//
//  Copyright © 2021 Secure Mobile Networking Lab (SEEMOO)
//  Copyright © 2021 The Open Wireless Link Project
//
//  SPDX-License-Identifier: AGPL-3.0-only
//

import AppKit
import Cocoa
import Foundation
import Vapor


class AppDelegate: NSObject, NSApplicationDelegate {
    
    let vaporThread = DispatchQueue(label: "Vapor", qos: DispatchQoS.default)

    func applicationDidFinishLaunching(_ aNotification: Notification) {
        print("Started")
        AnisetteDataManager.shared.requestAnisetteData { result in
            print("feteched anisette data: \(result)")
        }
        
        vaporThread.async {
            self.startVapor()
        }
    }
    
    func startVapor() {
        print("starting vapor....")
        do {

            let app = try Application(.detect())
            app.http.server.configuration.port = 80
            app.http.server.configuration.hostname = "0.0.0.0"
            
            defer { app.shutdown() }

            app.get("test") { req -> String in
                print("Received report ids")
                return "ola"
            }
            //Routes
            app.post("getLocationReports") { req -> EventLoopFuture<Response> in
                let reportIds = try req.content.decode(GetReportsBody.self)
                print("Received report ids")
                return ServerReportsFetcher.fetchReports(for: reportIds.ids, with: req)
            }
            
            app.post("getAndDecryptReports") { req -> EventLoopFuture<Response> in
                let requestBody = try req.content.decode(DecryptReportsRequestBody.self)
                print("Received ids with private keys")
                let promise = req.eventLoop.makePromise(of: Response.self)
                
                let result = ServerReportsFetcher.fetchReports(for: requestBody.ids.map({$0.id}), with: req)
                    .flatMapThrowing { serverResponse -> EventLoopFuture<Response> in
                        //Now try to decrypt the received reports
                        guard let responseData = serverResponse.body.data else {
                            throw Abort.init(.notAcceptable)
                        }
                        let findMyResults = try JSONDecoder().decode(FindMyReportResults.self, from: responseData)
                        let decryptedReports = ServerReportsFetcher.decryptReports(findMyResults: findMyResults, idsAndKeys: requestBody.ids)
                        
                        return decryptedReports.encodeResponse(status: .ok, for: req)
                    }
                
                result.whenSuccess { futureResponse in
                    futureResponse.whenSuccess { response in
                        promise.succeed(response)
                    }
                    futureResponse.whenFailure { error in
                        promise.fail(error)
                    }
                }
                result.whenFailure { error in
                    promise.fail(error)
                }
                
                return promise.futureResult
            }
            
            try app.run()
            print("Started")
        } catch {
            fatalError("Vapor failed to start \(String(describing: error))")
        }
    }

    func applicationWillTerminate(_ aNotification: Notification) {
        // Insert code here to tear down your application
        print("chao")
    }

    func applicationSupportsSecureRestorableState(_ app: NSApplication) -> Bool {
        return true
    }

}
