Fn-0000000000000000:
  GetUID(0)
  Commit(0,0)
  SetMem(15,1)
  While(15)
    Pull(0,0)
    SetOpinion(0)
    Output(0,0)
    BroadcastMsg(0,0,0)[1111111111111111]
    RetrieveMsg
  Close

Fn-1111111111111111:
  Pull(0,0)
  Input(0,1)
  TestLess(1,0,2)
  If(2)
    Commit(1,0)
