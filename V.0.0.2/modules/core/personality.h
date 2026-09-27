#pragma once

struct PersonalityProfile {
  const char* id;
  const char* displayName;
  const char* responseDirection;
  const char* faceModule;
};

enum Personality {
  PERSONALITY_CALM,
  PERSONALITY_ROCKY,
  PERSONALITY_ENGINEER,
  PERSONALITY_SPARTAN,
  PERSONALITY_ISABELLA
};

inline const PersonalityProfile& deskbotPersonalityProfile(Personality personality) {
  static const PersonalityProfile profiles[] = {
    {"calm", "Calm", "warm, concise, reassuring", "procedural"},
    {"rocky", "Rocky", "curious, friendly, original alien-inspired", "rocky"},
    {"engineer", "Engineer", "precise electronics and coding guidance", "procedural"},
    {"spartan", "Spartan", "disciplined, wise, motivating", "spartan"},
    {"isabella", "Isabella", "soft, energetic, expressive, original", "isabella"}
  };
  return profiles[static_cast<int>(personality)];
}
