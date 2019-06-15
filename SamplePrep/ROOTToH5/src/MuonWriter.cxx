// this class's header
#include "../headers/MuonWriter.h"

// EDM things
#include "xAODMuon/MuonContainer.h"
#include "xAODMuon/Muon.h"
#include "xAODTruth/xAODTruthHelpers.h"
#include "MuonSelectorTools/MuonSelectionTool.h"
#include "xAODTracking/TrackParticlexAODHelpers.h"

// HDF5 things
#include "HDF5Utils/HdfTuple.h"
#include "H5Cpp.h"

MuonWriter::MuonWriter(H5::Group& output_group):
    m_muon_idx(1),
    m_writer(nullptr)
{

    // muon selection
    m_muonSelectionTool = new CP::MuonSelectionTool("MuonObject_MuonSelectionTool");
    m_muonSelectionTool->initialize();

    // define the variable filling functions. Each function takes no
    // arguments, but includes a pointer to the class instance, and by
    // extension to the current muon.
    H5Utils::VariableFillers fillers;

    fillers.add<int>("pdgID",
        [this]() {
            return 13;
        }
    );
    fillers.add<float>("pT",
        [this]() {
            size_t idx = this->m_muon_idx.at(0);
            if (this->m_current_muons.size() <= idx) return NAN;
            return (float)(this->m_current_muons.at(idx)->pt());
        }
    );
    fillers.add<float>("eta",
        [this]() {
            size_t idx = this->m_muon_idx.at(0);
            if (this->m_current_muons.size() <= idx) return NAN;
            return (float)(this->m_current_muons.at(idx)->eta());
        }
    );
    fillers.add<float>("phi",
        [this]() {
            size_t idx = this->m_muon_idx.at(0);
            if (this->m_current_muons.size() <= idx) return NAN;
            return (float)(this->m_current_muons.at(idx)->phi());
        }
    );
    fillers.add<float>("d0",
        [this]() {
            size_t idx = this->m_muon_idx.at(0);
            if (this->m_current_muons.size() <= idx) return NAN;
            return (float)(this->m_current_muons.at(idx)->trackParticle(xAOD::Muon::InnerDetectorTrackParticle)->d0());
        }
    );
    fillers.add<float>("d0_over_sigd0",
        [this]() {
            size_t idx = this->m_muon_idx.at(0);
            if (this->m_current_muons.size() <= idx) return NAN;
            return (float)(xAOD::TrackingHelpers::d0significance(this->m_current_muons.at(idx)->trackParticle(xAOD::Muon::InnerDetectorTrackParticle)));
        }
    );
    fillers.add<float>("z0",
        [this]() {
            size_t idx = this->m_muon_idx.at(0);
            if (this->m_current_muons.size() <= idx) return NAN;
            return (float)(this->m_current_muons.at(idx)->trackParticle(xAOD::Muon::InnerDetectorTrackParticle)->z0());
        }
    );
    fillers.add<float>("dz0",
        [this]() {
            size_t idx = this->m_muon_idx.at(0);
            if (this->m_current_muons.size() <= idx) return NAN;
            return (float)(this->m_current_muons.at(idx)->trackParticle(xAOD::Muon::InnerDetectorTrackParticle)->z0() - this->m_primary_vertices_z0.at(idx));
        }
    );
    fillers.add<float>("ptcone20",
        [this]() {
            size_t idx = this->m_muon_idx.at(0);
            if (this->m_current_muons.size() <= idx) return NAN;
            float ptcone20 = 0.0;
            this->m_current_muons.at(idx)->isolation(ptcone20,xAOD::Iso::ptcone20);
            return ptcone20;
        }
    );
    fillers.add<float>("ptcone30",
        [this]() {
            size_t idx = this->m_muon_idx.at(0);
            if (this->m_current_muons.size() <= idx) return NAN;
            float ptcone30 = 0.0;
            this->m_current_muons.at(idx)->isolation(ptcone30,xAOD::Iso::ptcone30);
            return ptcone30;
        }
    );
    fillers.add<float>("ptcone40",
        [this]() {
            size_t idx = this->m_muon_idx.at(0);
            if (this->m_current_muons.size() <= idx) return NAN;
            float ptcone40 = 0.0;
            this->m_current_muons.at(idx)->isolation(ptcone40,xAOD::Iso::ptcone40);
            return ptcone40;
        }
    );
    fillers.add<float>("ptvarcone20",
        [this]() {
            size_t idx = this->m_muon_idx.at(0);
            if (this->m_current_muons.size() <= idx) return NAN;
            float ptvarcone20 = 0.0;
            this->m_current_muons.at(idx)->isolation(ptvarcone20,xAOD::Iso::ptvarcone20);
            return ptvarcone20;
        }
    );
    fillers.add<float>("ptvarcone30",
        [this]() {
            size_t idx = this->m_muon_idx.at(0);
            if (this->m_current_muons.size() <= idx) return NAN;
            float ptvarcone30 = 0.0;
            this->m_current_muons.at(idx)->isolation(ptvarcone30,xAOD::Iso::ptvarcone30);
            return ptvarcone30;
        }
    );
    fillers.add<float>("ptvarcone40",
        [this]() {
            size_t idx = this->m_muon_idx.at(0);
            if (this->m_current_muons.size() <= idx) return NAN;
            float ptvarcone40 = 0.0;
            this->m_current_muons.at(idx)->isolation(ptvarcone40,xAOD::Iso::ptvarcone40);
            return ptvarcone40;
        }
    );
    fillers.add<int>("truth_type",
        [this]() -> int {
            size_t idx = this->m_muon_idx.at(0);
            if (this->m_current_muons.size() <= idx) return NAN;
            const xAOD::TrackParticle* track = this->m_current_muons.at(idx)->trackParticle(xAOD::Muon::InnerDetectorTrackParticle);
            return (int)(xAOD::TruthHelpers::getParticleTruthType(*track));
            // 2 = real prompt, 3 = HF
        }
    );
    fillers.add<float>("PLT",
        [this]() {
            size_t idx = this->m_muon_idx.at(0);
            if (this->m_current_muons.size() <= idx) return NAN;
            SG::AuxElement::ConstAccessor<float> accessPromptVar("PromptLeptonVeto");
            const xAOD::IParticle *particle_pointer = NULL;
            particle_pointer = this->m_current_muons.at(idx);
            return accessPromptVar(*particle_pointer);
        }
    );

    // Save up to 20 muons per event
    m_writer = new H5Utils::WriterXd(output_group, "muons", fillers, {20});
}

MuonWriter::~MuonWriter() {
    if (m_writer) m_writer->flush();
    delete m_writer;
}

void MuonWriter::filter_muons_first_stage(const xAOD::MuonContainer& muons) {
    m_current_muons.clear();
    for (const xAOD::Muon *muon : muons) {
        // check that muon won't segfault
        if (muon == NULL) continue;
        if (muon->trackParticle(xAOD::Muon::InnerDetectorTrackParticle) == NULL) continue;
        // check that muon passes selections
        xAOD::Muon::Quality muonQuality = m_muonSelectionTool->getQuality(*muon);
        if (muonQuality < xAOD::Muon::Medium) continue;
        // store muons
        m_current_muons.push_back(muon);
    }
}

void MuonWriter::extract_vertex_z0(const xAOD::VertexContainer& primary_vertices) {
    m_primary_vertices_z0.clear();
    for (const xAOD::Vertex *vertex : primary_vertices) {
        m_primary_vertices_z0.push_back(vertex->z());
    }
}

void MuonWriter::write(const xAOD::MuonContainer& muons, const xAOD::VertexContainer& primary_vertices) {

    // muon selection
    filter_muons_first_stage(muons);

    // extract primary vertex z0 values
    extract_vertex_z0(primary_vertices);

    // sort muons by descending pT
    std::sort(m_current_muons.begin(), m_current_muons.end(),
        [](const auto* t1, const auto* t2) {
          return t1->pt() > t2->pt();
    });

    // write muons
    m_writer->fillWhileIncrementing(m_muon_idx);
}