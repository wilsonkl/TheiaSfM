// Copyright (C) 2014 The Regents of the University of California (Regents).
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
//
//     * Neither the name of The Regents or University of California nor the
//       names of its contributors may be used to endorse or promote products
//       derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Please contact the author of this library if you have any questions.
// Author: Chris Sweeney (cmsweeney@cs.ucsb.edu)

#ifndef THEIA_MATCHING_FEATURE_MATCHER_H_
#define THEIA_MATCHING_FEATURE_MATCHER_H_

#include <Eigen/Core>

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "theia/matching/feature_matcher_options.h"
#include "theia/util/util.h"

namespace theia {
class FeaturesAndMatchesDatabase;
class Keypoint;
struct ImagePairMatch;
struct IndexedFeatureMatche;
struct KeypointsAndDescriptors;

// Class for matching features between images. The intended use for these
// classes is for matching photos in image collections, so all pairwise matches
// are computed. Matching with geometric verification is also possible. Typical
// use case is:
//   FeatureMatcherOptions matcher_options;
//   FeatureMatcher matcher(matcher_options);
//   for (int i = 0; i < num_images_to_match; i++) {
//     matcher.AddImage(image_names[i], keypoints[i], descriptors[i]);
//     // Or, you could add the image with known intrinsics for use during
//     // geometric verification.
//     matcher.AddImage(image_names[i], keypoints[i],
//                      descriptors[i], intrinsics[i]);
//   }
//   std::vector<ImagePairMatch> matches;
//   matcher.MatchImages(&matches);
//   // Or, with geometric verification:
//   VerifyTwoViewMatchesOptions geometric_verification_options;
//   matcher.MatchImages(geometric_verification_options, &matches);
//
// The matches and match quality depend on the options passed to the feature
// matching.
class FeatureMatcher {
 public:
  FeatureMatcher(const FeatureMatcherOptions& matcher_options,
                 FeaturesAndMatchesDatabase* feature_and_matches_db);
  virtual ~FeatureMatcher();

  // If features have been written to disk, the matcher can directly work with
  // them from the feature files so that you do not have to "add" them to the
  // matcher. This assumes that feature files have been written in the format:
  //
  //     //keypoints_and_descriptors_output_dir/image_name.features
  //
  // Care must be taken to ensure that the filenames are formatted correctly.
  virtual void AddImage(const std::string& image_name);

  // This method is essentially the same as AddImage() but run in batch.
  virtual void AddImages(const std::vector<std::string>& image_names);
  // Matches features between all images. No geometric verification is
  // performed. Only the matches which pass the have greater than
  // min_num_feature_matches are returned.
  virtual void MatchImages();

  // Set the image pairs that will be matched when MatchImages or
  // MatchImagesWithGeometricVerification is called. This is an optional method;
  // if it is not called, then all possible image-to-image pairs will be
  // matched. The vector should contain unique pairs of image names that should
  // be matched.
  virtual void SetImagePairsToMatch(
      const std::vector<std::pair<std::string, std::string> >& pairs_to_match);

 protected:
  // NOTE: This method should be overridden in the subclass implementations!
  // Returns true if the image pair is a valid match.
  virtual bool MatchImagePair(
      const KeypointsAndDescriptors& features1,
      const KeypointsAndDescriptors& features2,
      std::vector<IndexedFeatureMatch>* matched_features) = 0;

  // Performs matching and geometric verification (if desired) on the
  // pairs_to_match_ between the specified indices. This is useful for thread
  // pooling.
  virtual void MatchAndVerifyImagePairs(const int start_index,
                                        const int end_index);

  // Performs geometric verification. By making this a virtual method, derived
  // classes may implement custom verification methods (e.g., if rotations are
  // known then custom solvers can be used to solve for only the relative
  // translations).
  virtual bool GeometricVerification(
      const KeypointsAndDescriptors& features1,
      const KeypointsAndDescriptors& features2,
      const std::vector<IndexedFeatureMatch>& putative_matches,
      ImagePairMatch* image_pair_match);

  // Each Threadpool worker will perform matching on this many image pairs.  It
  // is more efficient to let each thread compute multiple matches at a time
  // than add each matching task to the pool. This is sort of like OpenMP's
  // dynamic schedule in that it is able to balance threads fairly efficiently.
  const int kMaxThreadingStepSize_ = 20;

  FeatureMatcherOptions options_;

  // A container for the image names.
  std::vector<std::string> image_names_;

  // DB to store and retrieve features and matches.
  FeaturesAndMatchesDatabase* feature_and_matches_db_;

  // Pairs that we will perform matching on.
  std::vector<std::pair<std::string, std::string> > pairs_to_match_;

 private:
  DISALLOW_COPY_AND_ASSIGN(FeatureMatcher);
};

}  // namespace theia

#endif  // THEIA_MATCHING_FEATURE_MATCHER_H_
