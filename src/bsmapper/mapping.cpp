#include "mapping.hpp"

#include <queue>
using std::priority_queue;

string ReverseComplimentStrand(const string& read) {
  string reverse_complement_read;
  uint32_t read_len = read.size();
  for (uint32_t i = 0; i < read_len; ++i) {
    reverse_complement_read += complimentBase(read[read_len - i - 1]);
  }
  return reverse_complement_read;
}

/* for bisulfite sequence mapping, Cs are transfered to Ts*/
void C2T(const string& orginal_read, const uint32_t& read_len, string& read) {
  for (uint32_t i = 0; i < read_len; ++i) {
    if ('N' == orginal_read[i]) {
      read += getNT(3);  // in rampbs N set to 3.
    } else if ('C' == orginal_read[i]) {
      read += 'T';
    } else {
      read += orginal_read[i];
    }
  }
}

uint32_t LowerBound(uint32_t low, uint32_t high, const char& chr,
                    const uint32_t& cmp_pos,
                    const vector<GenomePosition>& positions,
                    const Genome& genome) {
  uint32_t mid = 0;
  while (low < high) {
    mid = (low + high) / 2;
    char c = genome[positions[mid].chrom_id].sequence[positions[mid].chrom_pos
        + F2SEEDPAOSITION[cmp_pos]];
    if (c >= chr) {
      high = mid;
    } else {
      low = mid + 1;
    }
  }
  return low;
}

uint32_t UpperBound(uint32_t low, uint32_t high, const char& chr,
                    const uint32_t& cmp_pos,
                    const vector<GenomePosition>& positions,
                    const Genome& genome) {
  uint32_t mid = 0;
  while (low < high) {
    mid = (low + high + 1) / 2;
    char c = genome[positions[mid].chrom_id].sequence[positions[mid].chrom_pos
        + F2SEEDPAOSITION[cmp_pos]];
    if (c <= chr) {
      low = mid;
    } else {
      high = mid - 1;
    }
  }
  return low;
}

void GetRegion(const string& read, const vector<GenomePosition>& positions,
               const Genome& genome, const uint32_t& seed_length,
               pair<uint32_t, uint32_t>& region) {
  region.first = 1;
  region.second = 0;

  if (positions.size() == 0)
    return;

  uint32_t l = 0, u = positions.size() - 1;
  for (uint32_t p = F2SEEDWIGTH; p < seed_length; ++p) {
    l = LowerBound(l, u, read[F2SEEDPAOSITION[p]], p, positions, genome);
    u = UpperBound(l, u, read[F2SEEDPAOSITION[p]], p, positions, genome);
  }
  if (l > u)
    return;

  region.first = l;
  region.second = u;
}

void SingleEndMapping(const string& orginal_read, const Genome& genome,
                      const HashTable& hash_table, BestMatch& best_match,
                      const uint32_t& seed_length) {
  uint32_t read_len = orginal_read.size();
  if (read_len < HASHLEN)
    return;

  string read;
  C2T(orginal_read, read_len, read);

  uint32_t hash_value = getHashValue(read.c_str());
  HashTable::const_iterator it = hash_table.find(hash_value);
  if (it == hash_table.end())
    return;

  pair<uint32_t, uint32_t> region;
  GetRegion(read, it->second, genome, seed_length, region);

  for (uint32_t j = region.first; j <= region.second; ++j) {
    const Chromosome& chrom = genome[it->second[j].chrom_id];
    if (it->second[j].chrom_pos + read_len >= chrom.length)
      return;

    /* check the position */
    uint32_t num_of_mismatch = 0;
    for (uint32_t q = it->second[j].chrom_pos + HASHLEN, p = HASHLEN;
        p < read_len; ++q, ++p) {
      if (chrom.sequence[q] != read[p]) {
        num_of_mismatch++;
      }
      if (num_of_mismatch > best_match.mismatch)
        break;
    }

    if (num_of_mismatch < best_match.mismatch) {
      best_match = BestMatch(it->second[j].chrom_id, it->second[j].chrom_pos, 1,
                             num_of_mismatch);
    } else if (best_match.mismatch == num_of_mismatch) {
      best_match.times++;
    }
  }
}
