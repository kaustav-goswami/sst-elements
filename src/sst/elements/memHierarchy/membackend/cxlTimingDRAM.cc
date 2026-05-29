#include <sst/core/sst_config.h>
#include "cxlTimingDRAM.h"
#include <sst/elements/memHierarchy/util.h>

using namespace SST;
using namespace SST::MemHierarchy;

CXLTimingDRAM::CXLTimingDRAM(ComponentId_t id, Params &params) : TimingDRAM(id, params) {
    numNodes = params.find<uint32_t>("num_nodes", 100);
    nodePrefix = params.find<std::string>("node_prefix", "gem5_");
    nodeSuffix = params.find<std::string>("node_suffix", ".remote_memory_port"); // NEW

    invPort = configureLink("cxl_inv_port", new Event::Handler<CXLTimingDRAM>(this, &CXLTimingDRAM::handleInvAck));
}
bool CXLTimingDRAM::issueRequest(ReqId reqId, Addr addr, bool isWrite, unsigned numBytes) {
    Addr cacheLineAddr = addr & ~(63ULL); 
    uint64_t requestor = getRequestorID(reqId); 

    // output->verbose(
    //     CALL_INFO, 2, DBG_MASK, "isWrite=%d reqId=%" PRIu64 " addr=%#" PRIx64 "\n",
    //     isWrite,requestor,addr);

    if (isWrite) {
        // Send back invalidates to all OTHER nodes that hold this line
        for (uint64_t i = 0 ; i < numNodes ; i++) {
            if (i != requestor)
                sendBackInvalidate(cacheLineAddr, i);
        } 
    //     if (directory.find(cacheLineAddr) != directory.end()) {
    //         for (const uint64_t& owner : directory[cacheLineAddr]) {
    //             if (owner != requestor) {
    //                 sendBackInvalidate(cacheLineAddr, owner);
    //             }
    //         }
    //     }
        
    //     // Requester gets exclusive ownership
    //     directory[cacheLineAddr].clear();
    //     directory[cacheLineAddr].insert(requestor);
    // } else {
    //     // On read: add requester to sharers list
    //     directory[cacheLineAddr].insert(requestor);
    }


    // if (isWrite) {
    //     // Send back invalidates to all OTHER nodes that hold this line
    //     if (directory.find(cacheLineAddr) != directory.end()) {
    //         for (const uint64_t& owner : directory[cacheLineAddr]) {
    //             if (owner != requestor) {
    //                 sendBackInvalidate(cacheLineAddr, owner);
    //             }
    //         }
    //     }
        
    //     // Requester gets exclusive ownership
    //     directory[cacheLineAddr].clear();
    //     directory[cacheLineAddr].insert(requestor);
    // } else {
    //     // On read: add requester to sharers list
    //     directory[cacheLineAddr].insert(requestor);
    // }

    return TimingDRAM::issueRequest(reqId, addr, isWrite, numBytes);
}

uint64_t CXLTimingDRAM::getRequestorID(ReqId reqId) {
    // IMPORTANT: Ensure your setup accurately maps ReqId to the source node ID (0 to numNodes-1).
    // This modulo is a placeholder. If MemController issues sequential ReqIds, this won't work.
    // You may need to pass metadata from the bridge if ReqId doesn't correlate to the node.
    return reqId % numNodes; 
}

// void CXLTimingDRAM::sendBackInvalidate(Addr lineAddr, uint64_t targetNode) {
//     if (!invPort) return;
    
//     // Construct the destination dynamically (e.g., "gem5_0", "gem5_42")
//     // This will now dynamically build "gem5_1.remote_memory_port"
//     std::string targetName = nodePrefix + std::to_string(targetNode) + nodeSuffix;
    
//     // Create the event with the correct signature
//     MemEvent* invEvent = new MemEvent(getName(), lineAddr, lineAddr, Command::Inv, 64);
    
//     // Target the specific node
//     invEvent->setDst(targetName); 
    
//     invPort->send(invEvent);
// }

void CXLTimingDRAM::sendBackInvalidate(Addr lineAddr, uint64_t targetNode) {
    if (!invPort) {
        // output->verbose(CALL_INFO, 1, DBG_MASK, 
        //     "ERROR: invPort is null, cannot send invalidate\n");
        return;
    }
    
    // ✅ FIX: Build ONLY the component name (no port suffix)
    // SST routing uses component names; port is determined by link wiring
    std::string targetComponentName = nodePrefix + std::to_string(targetNode) + nodeSuffix;
    
    // output->verbose(CALL_INFO, 3, DBG_MASK, 
    //     "Sending back-invalidate: addr=%#" PRIx64 " -> component='%s' (full target was '%s%s')\n",
    //     lineAddr, targetComponentName.c_str(), 
    //     nodePrefix.c_str(), std::to_string(targetNode).c_str());
    
    // Create the invalidate event
    MemEvent* invEvent = new MemEvent(
        getName(),           // src: this backend's name
        lineAddr,            // virtual address (if used)
        lineAddr,            // base address (cache-line aligned)
        Command::Inv,        // invalidate command
        64                   // payload size (cache line)
    );
    
    // ✅ CRITICAL FIX: Set destination to COMPONENT NAME ONLY
    invEvent->setDst(targetComponentName);
    
    // Ensure global addressing for cross-component routing
    invEvent->setAddrGlobal(true);
    
    // Optional: Add metadata for debugging
    invEvent->setRqstr(getName());
    // invEvent->setInstCmdType(CommandClass::Memory);
    
    // Send via the configured link
    // memHierarchy will route to the port connected to this link
    invPort->send(invEvent);
    
    // output->verbose(CALL_INFO, 3, DBG_MASK, 
    //     "✓ Sent Inv event to component '%s' for addr %#" PRIx64 "\n",
    //     targetComponentName.c_str(), lineAddr);
}

void CXLTimingDRAM::handleInvAck(SST::Event* ev) {
    MemEvent* ack = static_cast<MemEvent*>(ev);
    delete ack;
}
