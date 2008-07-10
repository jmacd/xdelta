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
    DELETE = 3,
    MOVE = 4,
    COPY = 5,
    OVERWRITE = 6,
  };

  // Constructor for modify, add, delete.
  Change(Kind kind, xoff_t size, xoff_t addr1)
    : kind(kind),
      size(size),
      addr1(addr1),
    insert(NULL) { 
    CHECK(kind != MOVE && kind != COPY && kind != OVERWRITE);
  }

  // Constructor for modify, add w/ provided data.
  Change(Kind kind, xoff_t size, xoff_t addr1, Segment *insert)
    : kind(kind),
      size(size),
      addr1(addr1),
      insert(insert) { 
    CHECK(kind != MOVE && kind != COPY && kind != OVERWRITE);
  }

  // Constructor for move
  Change(Kind kind, xoff_t size, xoff_t addr1, xoff_t addr2)
    : kind(kind),
      size(size),
      addr1(addr1),
      addr2(addr2),
      insert(NULL) { 
    CHECK(kind == MOVE || kind == COPY || kind == OVERWRITE);
  }

  Kind kind;
  xoff_t size;
  xoff_t addr1;
  xoff_t addr2;
  Segment *insert;  // For modify and/or add
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

  static void OverwriteChange(const Change &ch, 
			      SegmentMap *table,
			      const SegmentMap *source_table,
			      MTRandom *rand);

  static void CopyChange(const Change &ch, 
			 SegmentMap *table,
			 const SegmentMap *source_table,
			 MTRandom *rand);

  static void AppendCopy(SegmentMap *table,
			 const SegmentMap *source_table,
			 xoff_t copy_offset, 
			 xoff_t append_offset, 
			 xoff_t length);

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
    tmp.clear();
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
    DeleteChange(ch, table, source_table, rand);
    break;
  case Change::COPY:
    CopyChange(ch, table, source_table, rand);
    break;
  case Change::MOVE:
    MoveChange(ch, table, source_table, rand);
    break;
  case Change::OVERWRITE:
    OverwriteChange(ch, table, source_table, rand);
    break;
  }
}

void ChangeListMutator::ModifyChange(const Change &ch, 
				     SegmentMap *table,
				     const SegmentMap *source_table,
				     MTRandom *rand) {
  xoff_t m_start = ch.addr1;
  xoff_t m_end = m_start + ch.size;
  xoff_t i_start = 0;
  xoff_t i_end = 0;

  for (SegmentMap::const_iterator iter(source_table->begin()); 
       iter != source_table->end();
       ++iter) {
    const Segment &seg = iter->second;
    i_start = iter->first;
    i_end = i_start + seg.Size();

    if (i_end <= m_start || i_start >= m_end) {
      table->insert(table->end(), make_pair(i_start, seg));
      continue;
    }

    if (i_start < m_start) {
      table->insert(table->end(), 
		    make_pair(i_start, 
			      seg.Subseg(0, m_start - i_start)));
    }

    // Insert the entire segment, even though it may extend into later
    // segments.  This condition avoids inserting it during later
    // segments.
    if (m_start >= i_start) {
      if (ch.insert != NULL) {
	table->insert(table->end(), make_pair(m_start, *ch.insert));
      } else {
	Segment part(m_end - m_start, rand);
	table->insert(table->end(), make_pair(m_start, part));
      }
    }

    if (i_end > m_end) {
      table->insert(table->end(), 
		    make_pair(m_end, 
			      seg.Subseg(m_end - i_start, i_end - m_end)));
    }
  }

  CHECK_LE(m_end, i_end);
}

void ChangeListMutator::AddChange(const Change &ch, 
				  SegmentMap *table,
				  const SegmentMap *source_table,
				  MTRandom *rand) {
  xoff_t m_start = ch.addr1;
  xoff_t i_start = 0;
  xoff_t i_end = 0;

  for (SegmentMap::const_iterator iter(source_table->begin()); 
       iter != source_table->end();
       ++iter) {
    const Segment &seg = iter->second;
    i_start = iter->first;
    i_end = i_start + seg.Size();

    if (i_end <= m_start) {
      table->insert(table->end(), make_pair(i_start, seg));
      continue;
    }

    if (i_start > m_start) {
      table->insert(table->end(), make_pair(i_start + ch.size, seg));
      continue;
    }

    if (i_start < m_start) {
      table->insert(table->end(), 
		    make_pair(i_start, 
			      seg.Subseg(0, m_start - i_start)));
    }

    if (ch.insert != NULL) {
      table->insert(table->end(), make_pair(m_start, *ch.insert));
    } else {
      Segment addseg(ch.size, rand);
      table->insert(table->end(), make_pair(m_start, addseg));
    }

    if (m_start < i_end) {
      table->insert(table->end(), 
		    make_pair(m_start + ch.size, 
			      seg.Subseg(m_start - i_start, i_end - m_start)));
    }
  }

  CHECK_LE(m_start, i_end);

  // Special case for add at end-of-input.
  if (m_start == i_end) {
    Segment addseg(ch.size, rand);
    table->insert(table->end(), make_pair(m_start, addseg));
  }
}

void ChangeListMutator::DeleteChange(const Change &ch, 
				     SegmentMap *table,
				     const SegmentMap *source_table,
				     MTRandom *rand) {
  xoff_t m_start = ch.addr1;
  xoff_t m_end = m_start + ch.size;
  xoff_t i_start = 0;
  xoff_t i_end = 0;

  for (SegmentMap::const_iterator iter(source_table->begin()); 
       iter != source_table->end();
       ++iter) {
    const Segment &seg = iter->second;
    i_start = iter->first;
    i_end = i_start + seg.Size();

    if (i_end <= m_start) {
      table->insert(table->end(), make_pair(i_start, seg));
      continue;
    }

    if (i_start >= m_end) {
      table->insert(table->end(), make_pair(i_start - ch.size, seg));
      continue;
    }

    if (i_start < m_start) {
      table->insert(table->end(), 
		    make_pair(i_start, 
			      seg.Subseg(0, m_start - i_start)));
    }

    if (i_end > m_end) {
      table->insert(table->end(), 
		    make_pair(m_end - ch.size, 
			      seg.Subseg(m_end - i_start, i_end - m_end)));
    }
  }

  CHECK_LT(m_start, i_end);
  CHECK_LE(m_end, i_end);
}

void ChangeListMutator::MoveChange(const Change &ch, 
				   SegmentMap *table,
				   const SegmentMap *source_table,
				   MTRandom *rand) {
  SegmentMap tmp;
  CHECK_NE(ch.addr1, ch.addr2);
  CopyChange(ch, &tmp, source_table, rand);
  Change d(Change::DELETE, ch.size, 
	   ch.addr1 < ch.addr2 ? ch.addr1 : ch.addr1 + ch.size);
  DeleteChange(d, table, &tmp, rand);
}

void ChangeListMutator::OverwriteChange(const Change &ch, 
				   SegmentMap *table,
				   const SegmentMap *source_table,
				   MTRandom *rand) {
  SegmentMap tmp;
  CHECK_NE(ch.addr1, ch.addr2);
  CopyChange(ch, &tmp, source_table, rand);
  Change d(Change::DELETE, ch.size, ch.addr2 + ch.size);
  DeleteChange(d, table, &tmp, rand);
}

void ChangeListMutator::CopyChange(const Change &ch, 
				   SegmentMap *table,
				   const SegmentMap *source_table,
				   MTRandom *ignore) {
  xoff_t m_start = ch.addr2;
  xoff_t c_start = ch.addr1;
  xoff_t i_start = 0;
  xoff_t i_end = 0;

  // Like AddChange() with AppendCopy instead of a random segment.
  for (SegmentMap::const_iterator iter(source_table->begin()); 
       iter != source_table->end();
       ++iter) {
    const Segment &seg = iter->second;
    i_start = iter->first;
    i_end = i_start + seg.Size();

    if (i_end <= m_start) {
      table->insert(table->end(), make_pair(i_start, seg));
      continue;
    }

    if (i_start > m_start) {
      table->insert(table->end(), make_pair(i_start + ch.size, seg));
      continue;
    }

    if (i_start < m_start) {
      table->insert(table->end(), 
		    make_pair(i_start, 
			      seg.Subseg(0, m_start - i_start)));
    }

    AppendCopy(table, source_table, c_start, m_start, ch.size);

    if (m_start < i_end) {
      table->insert(table->end(), 
		    make_pair(m_start + ch.size, 
			      seg.Subseg(m_start - i_start, i_end - m_start)));
    }
  }

  CHECK_LE(m_start, i_end);

  // Special case for copy to end-of-input.
  if (m_start == i_end) {
    AppendCopy(table, source_table, c_start, m_start, ch.size);
  }
}

void ChangeListMutator::AppendCopy(SegmentMap *table,
				   const SegmentMap *source_table,
				   xoff_t copy_offset, 
				   xoff_t append_offset,
				   xoff_t length) {
  SegmentMap::const_iterator pos(source_table->upper_bound(copy_offset));
  --pos;
  xoff_t got = 0;

  while (got < length) {
    size_t seg_offset = copy_offset - pos->first;
    size_t advance = min(pos->second.Size() - seg_offset, 
			 (size_t)(length - got));

    table->insert(table->end(), 
		  make_pair(append_offset,
			    pos->second.Subseg(seg_offset,
					       advance)));

    got += advance;
    copy_offset += advance;
    append_offset += advance;
    ++pos;
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
