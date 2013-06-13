#pragma once
#include "Base.h"
#include "VersionedAsset.h"

// Templatized to be able to create strong typedefs (c++ has weak ones)
template <int>
class BaseFileSetRequest : public BaseRequest
{
public:
	BaseFileSetRequest(const CommandArgs& args, UnityConnection& conn) : BaseRequest(args, conn)
	{
		assets.clear();
		UnityPipe& upipe = conn.Pipe();
		upipe >> assets;

		if (assets.empty())
		{
			assets.clear();
			upipe << assets;
			upipe.EndResponse();
			invalid = true;
		}
	}

	VersionedAssetList assets;
};

// Basic requests
typedef BaseFileSetRequest<0> StatusRequest;
typedef BaseFileSetRequest<1> AddRequest;
typedef BaseFileSetRequest<2> CheckoutRequest;
typedef BaseFileSetRequest<3> DeleteRequest;
typedef BaseFileSetRequest<4> GetLatestRequest;
typedef BaseFileSetRequest<5> LockRequest;
typedef BaseFileSetRequest<6> UnlockRequest;
typedef BaseFileSetRequest<7> ResolveRequest;
typedef BaseFileSetRequest<8> RevertRequest;
typedef BaseFileSetRequest<9> MoveRequest;
typedef BaseFileSetRequest<10> FileModeRequest;


template <class Req>
class BaseFileSetResponse : public BaseResponse
{
public:
    BaseFileSetResponse(Req& req) : request(req) {}

	void Write()
	{
		if (request.invalid)
			return;
		
		UnityPipe& upipe = request.conn.Pipe();
	
		upipe << assets;
		upipe.EndResponse();
	}

	Req& request;
	VersionedAssetList assets;
};


// Basic responses
typedef BaseFileSetResponse<StatusRequest> StatusResponse;
typedef BaseFileSetResponse<AddRequest> AddResponse;
typedef BaseFileSetResponse<CheckoutRequest> CheckoutResponse;
typedef BaseFileSetResponse<DeleteRequest> DeleteResponse;
typedef BaseFileSetResponse<GetLatestRequest> GetLatestResponse;
typedef BaseFileSetResponse<LockRequest> LockResponse;
typedef BaseFileSetResponse<UnlockRequest> UnlockResponse;
typedef BaseFileSetResponse<ResolveRequest> ResolveResponse;
typedef BaseFileSetResponse<RevertRequest> RevertResponse;
typedef BaseFileSetResponse<MoveRequest> MoveResponse;
typedef BaseFileSetResponse<FileModeRequest> FileModeResponse;
