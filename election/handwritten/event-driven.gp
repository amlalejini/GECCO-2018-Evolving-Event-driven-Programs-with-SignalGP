Fn-0000000000000000:
  GetUID(0)
  SetOpinion(0)
  SetMem(15,1)
  While(15)
    GetOpinion(0)
    Output(0,0)
    BroadcastMsg(0,0,0)[1111111111111111]
  Close

Fn-1111111111111111:
  Input(0,1)
  GetOpinion(0)
  TestLess(1,0,2)
  If(2)
    SetOpinion(1)