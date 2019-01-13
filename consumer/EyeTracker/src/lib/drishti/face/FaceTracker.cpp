/*! -*-c++-*-
  @file   FaceTracker.cpp
  @author David Hirvonen
  @brief  Declaration of a face landmark tracker.

  \copyright Copyright 2017 Elucideye, Inc. All rights reserved.
  \license{This project is released under the 3 Clause BSD License.}

*/

#include <android/log.h>

#include "drishti/face/FaceTracker.h" // FaceModel.h
#include "drishti/core/make_unique.h"
#include "drishti/core/hungarian.h"

DRISHTI_FACE_NAMESPACE_BEGIN

struct FaceTracker::Impl
{
    using FaceTrack = std::pair<drishti::face::FaceModel, FaceTracker::TrackInfo>;
    using FaceTrackVec = std::vector<FaceTrack>;
    using FaceModelVec = std::vector<drishti::face::FaceModel>;

    Impl(float costThreshold, std::size_t minTrackHits, std::size_t maxTrackMisses)
        : m_costThreshold(costThreshold)
        , m_minTrackHits(minTrackHits)
        , m_maxTrackMisses(maxTrackMisses)
    {
    }

    void update(const FaceModelVec& facesIn, FaceTrackVec& facesOut, double& x, double& y)
    {
        if (m_tracks.size() == 0)
        {
            // Initialize tracks:
            for (const auto& f : facesIn)
            {
                m_tracks.emplace_back(f, TrackInfo(m_id++));
            }
        }
        else
        {
            std::unordered_map<int, int> direct_assignment;
            std::unordered_map<int, int> reverse_assignment;
            std::vector<std::vector<double>> C;

            if (facesIn.size())
            {
                C = std::vector<std::vector<double>>(m_tracks.size(), std::vector<double>(facesIn.size()));
                for (int i = 0; i < m_tracks.size(); i++)
                {
                    for (int j = 0; j < facesIn.size(); j++)
                    {
                        C[i][j] = cv::norm(*m_tracks[i].first.eyesCenter - *facesIn[j].eyesCenter);
                    }
                }
                core::MinimizeLinearAssignment(C, direct_assignment, reverse_assignment);
            }

            // For all valid assignments we will either
            //  (1) stack a new track if the assignment cost is too high
            //  (2) extend an existing track
            // 
            // We will then start new tracks for all remaining unasigned detections:
            std::vector<std::uint8_t> hits(m_tracks.size(), 0), assigned(facesIn.size(), 0);
            for (const auto& m : direct_assignment)
            {
                assigned[m.second] = 1;

                // Create a new track, or update an old track:
                if (C[m.first][m.second] > m_costThreshold)
                {
                    m_tracks.emplace_back(facesIn[m.second], TrackInfo(m_id++));
                }
                else
                {
                    m_tracks[m.first].second.hit();
                    m_tracks[m.first].first = facesIn[m.second];
                    hits[m.first] = 1;
                }
            }

            for (std::size_t i = 0; i < hits.size(); i++)
            {
                if (!hits[i])
                {
                    m_tracks[i].second.miss();
                }
            }

            // ... finally we initialize new tracks for all unasigned detections...
            for (std::size_t i = 0; i < assigned.size(); i++)
            {
                if (!assigned[i])
                {
                    m_tracks.emplace_back(facesIn[i], TrackInfo(m_id++));
                }
            }
            
            // Prune old tracks:
            m_tracks.erase(std::remove_if(m_tracks.begin(), m_tracks.end(), [this](const FaceTrack& track) {
                return (track.second.misses >= m_maxTrackMisses);
            }), m_tracks.end());
        }

        // Return subset of remaining mature tracks:
        std::copy_if(m_tracks.begin(), m_tracks.end(), std::back_inserter(facesOut), [this](const FaceTrack& track) {
            return (track.second.age > m_minTrackHits);
        });

        if (facesIn.size() > 0) {
            // __android_log_print(ANDROID_LOG_INFO, "onDrawFrame: ", "%f, %f",
            //     facesIn[0].eyesCenter->x,
            //     facesIn[0].eyesCenter->y
            // );
            x = facesIn[0].eyesCenter->x;
            y = facesIn[0].eyesCenter->y;
        }
        else
        {
            x = 100;
            y = 100;
        }
    }

    float m_costThreshold = 0.15; // meters
    std::size_t m_minTrackHits = 3;
    std::size_t m_maxTrackMisses = 3;

    std::size_t m_id = 0;

    FaceTrackVec m_tracks;
};

FaceTracker::FaceTracker(float costThreshold, std::size_t minTrackHits, std::size_t maxTrackMisses)
{
    m_impl = drishti::core::make_unique<Impl>(costThreshold, minTrackHits, maxTrackMisses);
}

FaceTracker::~FaceTracker() = default;

void FaceTracker::operator()(const FaceModelVec& facesIn, FaceTrackVec& facesOut)
{
    m_impl->update(facesIn, facesOut, x, y);
}

DRISHTI_FACE_NAMESPACE_END
