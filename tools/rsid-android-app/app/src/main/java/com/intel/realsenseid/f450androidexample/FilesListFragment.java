package com.intel.realsenseid.f450androidexample;

import android.content.Context;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Toast;

import java.util.List;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

public class FilesListFragment extends Fragment {
    private static final String ARG_PATH = "com.intel.realsenseid.f450androidexample.fileLists.path";
    private FilesRecyclerAdapter mFilesAdapter;
    private OnViewItemClickListener mCallback;
    private String PATH;

    public interface OnViewItemClickListener {
        void onClick(FileModel fileModel);
        void onLongClick(FileModel fileModel);
    }

    public FilesListFragment(String path) {
        Bundle args = new Bundle();
        args.putString(ARG_PATH, path);
        setArguments(args);
    }

    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                             @Nullable Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_files_list, container, false);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        String filePath = getArguments().getString(ARG_PATH);
        if (filePath == null) {
            Toast.makeText(view.getContext(), "Path should not be null!", Toast.LENGTH_SHORT).show();
            return;
        }
        PATH = filePath;
        initViews(view);
    }

    @Override
    public void onAttach(@NonNull Context context) {
        super.onAttach(context);

        try {
            mCallback = (OnViewItemClickListener) context;
        } catch (Exception e) {
            throw new RuntimeException("OnViewItemClickListener interface not implemented by context");
        }
    }

    private void initViews(View view) {
        RecyclerView filesRecyclerView = view.findViewById(R.id.files_recycler_view);
        filesRecyclerView.setLayoutManager(new LinearLayoutManager(view.getContext()));
        mFilesAdapter = new FilesRecyclerAdapter(mCallback);
        filesRecyclerView.setAdapter(mFilesAdapter);
        updateDate(view);
    }

    private void updateDate(View view) {
        List<FileModel> files = FileUtils.getFileModelsFromFiles(FileUtils.getFilesFromPath(PATH));
        if (files.isEmpty()) {
            view.findViewById(R.id.empty_folder_layout).setVisibility(View.VISIBLE);
        } else {
            view.findViewById(R.id.empty_folder_layout).setVisibility(View.INVISIBLE);
        }
        mFilesAdapter.updateData(files);
    }
}