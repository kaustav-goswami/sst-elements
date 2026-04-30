#ifndef _CXL_TIMING_DRAM_H
#define _CXL_TIMING_DRAM_H

#include <sst/elements/memHierarchy/membackend/timingDRAMBackend.h>
#include <sst/elements/memHierarchy/memEvent.h>
#include <unordered_map>
#include <unordered_set>
#include <string>

namespace SST {
namespace MemHierarchy {

class CXLTimingDRAM : public TimingDRAM {
public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        CXLTimingDRAM,
        "memHierarchy",
        "cxlTimingDRAM",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "CXL-extended TimingDRAM with back invalidations",
        SST::MemHierarchy::MemBackend
    )

    SST_ELI_DOCUMENT_PORTS(
        {"cxl_inv_port", "Port for sending CXL back invalidates to gem5 nodes", { "memHierarchy.MemEventBase" } }
    )

    // Register the new parameters
    SST_ELI_DOCUMENT_PARAMS(
        {"num_nodes", "Total number of connected gem5 nodes", "2"},
        {"node_prefix", "Prefix for the gem5 component names (e.g., 'gem5_')", "gem5_"},
        {"node_suffix", "Suffix for the gem5 memory port", ".remote_memory_port"} // NEW
    )

    CXLTimingDRAM(ComponentId_t id, Params &params);
    virtual ~CXLTimingDRAM() {}

    virtual bool issueRequest(ReqId reqId, Addr addr, bool isWrite, unsigned numBytes) override;

private:
    SST::Link* invPort;
    std::unordered_map<Addr, std::unordered_set<uint64_t>> directory;
    
    uint32_t numNodes;
    std::string nodePrefix;
    std::string nodeSuffix;

    uint64_t getRequestorID(ReqId reqId);
    void sendBackInvalidate(Addr lineAddr, uint64_t targetNode);
    void handleInvAck(SST::Event* ev);
};

} // namespace MemHierarchy
} // namespace SST

#endif
