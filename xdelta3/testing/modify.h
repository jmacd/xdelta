// -*- Mode: C++ -*-
namespace regtest {

class Mutator {
public:
  virtual ~Mutator() { }
  virtual void Mutate(SegmentMap *table, 
		      const SegmentMap *source_table, 
		      MTRandom *rand) const = 0;
};

class Change {
public:
  enum Kind {
    MODIFY = 1,
    ADD = 2,
    MOVE = 3,
    DELETE = 4,
  };

  // Constructor for modify, add, delete.
  Change(Kind kind, xoff_t size, xoff_t addr1)
    : kind(kind),
      size(size),
      addr1(addr1) { 
    CHECK_NE(kind, MOVE);
  }

  // Constructor for move
  Change(Kind kind, xoff_t size, xoff_t addr1, xoff_t addr2)
    : kind(kind),
      size(size),
      addr1(addr1) { 
    CHECK_EQ(kind, MOVE);
  }

  Kind kind;
  xoff_t size;
  xoff_t addr1;
  xoff_t addr2;
};

typedef list<Change> ChangeList;

class ChangeListMutator : public Mutator {
public:
  ChangeListMutator(const ChangeList &cl)
    : cl_(cl) { }

  ChangeListMutator() { }
  
  void Mutate(SegmentMap *table,
	      const SegmentMap *source_table,
	      MTRandom *rand) const;

  static void Mutate(const Change &ch, 
		     SegmentMap *table,
		     const SegmentMap *source_table,
		     MTRandom *rand);

  static void AddChange(const Change &ch, 
			SegmentMap *table,
			const SegmentMap *source_table,
			MTRandom *rand);

  static void ModifyChange(const Change &ch, 
			   SegmentMap *table,
			   const SegmentMap *source_table,
			   MTRandom *rand);

  static void DeleteChange(const Change &ch, 
			   SegmentMap *table,
			   const SegmentMap *source_table,
			   MTRandom *rand);

  static void MoveChange(const Change &ch, 
			 SegmentMap *table,
			 const SegmentMap *source_table,
			 MTRandom *rand);

  ChangeList* Changes() {
    return &cl_;
  }

  const ChangeList* Changes() const {
    return &cl_;
  }

private:
  ChangeList cl_;
};

void ChangeListMutator::Mutate(SegmentMap *table,
			       const SegmentMap *source_table,
			       MTRandom *rand) const {
  // The speed of processing gigabytes of data is so slow compared with
  // these table-copy operations, no attempt to make this fast.
  SegmentMap tmp;

  for (ChangeList::const_iterator iter(cl_.begin()); iter != cl_.end(); ++iter) {
    const Change &ch = *iter;
    Mutate(ch, &tmp, source_table, rand);
    tmp.swap(*table);
    source_table = table;
  }
}
  
void ChangeListMutator::Mutate(const Change &ch, 
			       SegmentMap *table,
			       const SegmentMap *source_table,
			       MTRandom *rand) {
  switch (ch.kind) {
  case Change::ADD:
    AddChange(ch, table, source_table, rand);
    break;
  case Change::MODIFY:
    ModifyChange(ch, table, source_table, rand);
    break;
  case Change::DELETE:
    //DeleteChange(ch, table, source_table, rand);
    break;
  case Change::MOVE:
    //MoveChange(ch, table, source_table, rand);
    break;
  }
}  

void ChangeListMutator::ModifyChange(const Change &ch, 
				     SegmentMap *table,
				     const SegmentMap *source_table,
				     MTRandom *rand) {
  xoff_t m_start = ch.addr1;
  xoff_t m_end = m_start + ch.size;

  for (SegmentMap::const_iterator iter(source_table->begin()); 
       iter != source_table->end();
       ++iter) {
    const Segment &seg = iter->second;
    xoff_t i_start = iter->first;
    xoff_t i_end = i_start + seg.length;

    if (i_end <= m_start || i_start >= m_end) {
      table->insert(table->end(), make_pair(i_start, seg));
      continue;
    }

    if (i_start < m_start) {
      Segment before(seg.seed, m_start - i_start, seg.seed_offset);
      table->insert(table->end(), make_pair(i_start, before));
    }

    // Insert the entire segment, even though it may extend into later
    // segments.  This condition avoids inserting it during later
    // segments.
    if (m_start >= i_start) {
      Segment part(rand->Rand32(), m_end - m_start);
      table->insert(table->end(), make_pair(m_start, part));
    }

    if (i_end > m_end) {
      Segment after(seg.seed, i_end - m_end, seg.seed_offset + (m_end - i_start));
      table->insert(table->end(), make_pair(m_end, after));
    }
  }
}

void ChangeListMutator::AddChange(const Change &ch, 
				  SegmentMap *table,
				  const SegmentMap *source_table,
				  MTRandom *rand) {
  xoff_t m_start = ch.addr1;

  for (SegmentMap::const_iterator iter(source_table->begin()); 
       iter != source_table->end();
       ++iter) {
    const Segment &seg = iter->second;
    xoff_t i_start = iter->first;
    xoff_t i_end = i_start + seg.length;

    if (i_end <= m_start) {
      table->insert(table->end(), make_pair(i_start, seg));
      continue;
    }

    if (i_start > m_start) {
      table->insert(table->end(), make_pair(i_start + ch.size, seg));
      continue;
    }

    if (i_start < m_start) {
      Segment before(seg.seed, m_start - i_start, seg.seed_offset);
      table->insert(table->end(), make_pair(i_start, before));
    }

    Segment addseg(rand->Rand32(), ch.size);
    table->insert(table->end(), make_pair(m_start, addseg));

    if (m_start < i_end) {
      Segment after(seg.seed, i_end - m_start, 
		    seg.seed_offset + (m_start - i_start));
      table->insert(table->end(), make_pair(m_start + ch.size, after));
    }
  }
}

class Modify1stByte : public Mutator {
public:
  void Mutate(SegmentMap *table, 
	      const SegmentMap *source_table, 
	      MTRandom *rand) const {
    ChangeListMutator::Mutate(Change(Change::MODIFY, 1, 0),
			      table, source_table, rand);
  }
};

}  // namespace regtest
