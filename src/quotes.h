#ifndef QUOTES_H
#define QUOTES_H

#include <Arduino.h>
// define an array of *short* bitcoin quotes from Satoshi
const char *quotes[] = {
    "The Times 03/Jan/2009 Chancellor on brink of second bailout for banks.\nSatoshi Nakamoto - Genesis Block",
    "I’m sure that in 20 years there will either be very large (bitcoin) transaction volume or no volume.\nSatoshi Nakamoto - Feb. 14, 2010",
    "How does everyone feel about the B symbol with the two lines through the outside? Can we live with that as our logo?\nSatoshi Nakamoto - Feb. 26, 2010",
    "The nature of Bitcoin is such that once version 0.1 was released, the core design was set in stone for the rest of its lifetime.\nSatoshi Nakamoto - June 17, 2010",
    "Lost coins only make everyone else’s coins worth slightly more. Think of it as a donation to everyone.\nSatoshi Nakamoto - June 21, 2010",
    "Sorry to be a wet blanket. Writing a description for (bitcoin) for general audiences is bloody hard. There’s nothing to relate it to.\nSatoshi Nakamoto - July 5, 2010",
    "Bitcoin is an implementation of Wei Dai‘s b-money proposal on Cypherpunks in 1998 and Nick Szabo’s Bitgold proposal.\nSatoshi Nakamoto - July 20, 2010",
    "If you don’t believe me or don’t get it, I don’t have time to try to convince you, sorry.\nSatoshi Nakamoto - July 29, 2010",
    "Sorry, I’ve been so busy lately I’ve been skimming messages and I still can’t keep up.\nSatoshi Nakamoto - Aug. 27, 2010",
    "Fears about securely buying domains with Bitcoins are a red herring. It’s easy to trade Bitcoins for other non-reputable commodities.\nSatoshi Nakamoto - Dec. 10, 2010",
    "I am not Dorian Nakamoto.\nSatoshi Nakamoto - Mar. 6, 2014"
};

/**
 * @brief Get a random quote
 * return const char*
 */
const char *getRandomQuote() {
  int quoteCount = sizeof(quotes) / sizeof(quotes[0]);
  int randomIndex = random(0, quoteCount);
  return quotes[randomIndex];
}

#endif